module;

#include <spdlog/spdlog.h>
#include <cld3/nnet_language_identifier.h>
#include <cld3/language_identifier_features.h>

export module ProblemAnalyzer;

import Tool;
import Dictionary;

export {

    struct ProblemCompareObj {
        bool use = false;
        CachePart base = CachePart::OrigText;
        CachePart check = CachePart::TransPreview;
    };

	struct Problems {
        ProblemCompareObj highFrequency;
        ProblemCompareObj punctMiss;
        ProblemCompareObj remainJp;
        ProblemCompareObj introLatin;
        ProblemCompareObj introHangul;
        ProblemCompareObj linebreakLost;
        ProblemCompareObj linebreakAdded;
        ProblemCompareObj longer;
        ProblemCompareObj strictlyLonger;
        ProblemCompareObj dictUnused;
        ProblemCompareObj notTargetLang;
	};

	class ProblemAnalyzer {

	private:

        std::vector<std::string> m_checks;
        std::shared_ptr<spdlog::logger> m_logger;
        double m_probabilityThreshold = 0.85;

		Problems m_problems;

	public:

        ProblemAnalyzer(std::shared_ptr<spdlog::logger> logger) : m_logger(logger) {}

		void loadProblems(const std::vector<std::string>& problemList, const std::string& punctSet, double langProbability);

        void overwriteCompareObj(const std::string& problemKey, const std::string& base, const std::string& check);

		void analyze(Sentence* sentence, GptDictionary& gptDict, const std::string& targetLang);
	};
}


module :private;

void ProblemAnalyzer::analyze(Sentence* sentence, GptDictionary& gptDict, const std::string& targetLang) {
    if (sentence->translated_preview.empty()) {
        if (!sentence->pre_processed_text.empty() && !sentence->pre_translated_text.empty()) {
            sentence->problem = "翻译为空";
        }
        else {
            sentence->problem.clear();
        }
        return;
    }
    if (sentence->translated_preview.starts_with("(Failed to translate)")) {
        sentence->problem = "翻译失败";
        return;
    }

    sentence->problem.clear();
    std::vector<std::string> problemList;

    // 1. 词频过高
    if (m_problems.highFrequency.use) {
        const std::string& origText = chooseStringRef(sentence, m_problems.highFrequency.base);
        const std::string& transView = chooseStringRef(sentence, m_problems.highFrequency.check);
        auto [mostWord, wordCount] = getMostCommonChar(transView);
        auto [mostWordOrg, wordCountOrg] = getMostCommonChar(origText);
        if (wordCount > 20 && wordCount > (wordCountOrg > 0 ? wordCountOrg * 2 : 20)) {
            problemList.push_back("词频过高-'" + mostWord + "'" + std::to_string(wordCount) + "次");
        }
    }

    // 2. 标点错漏
    if (m_problems.punctMiss.use) {
        const std::string& origText = chooseStringRef(sentence, m_problems.punctMiss.base);
        const std::string& transView = chooseStringRef(sentence, m_problems.punctMiss.check);
        for (const auto& check : m_checks) {
            bool orgHas = origText.find(check) != std::string::npos;
            bool transHas = transView.find(check) != std::string::npos;

            if (orgHas && !transHas) problemList.push_back("本有 " + check + " 符号");
            if (!orgHas && transHas) problemList.push_back("本无 " + check + " 符号");
        }
    }

    // 3. 残留日文
    if (m_problems.remainJp.use) {
        const std::string& transView = chooseStringRef(sentence, m_problems.remainJp.check);
        if (containsKana(transView)) {
            problemList.push_back("残留日文");
        }
    }

    if (m_problems.introLatin.use) {
        const std::string& origText = chooseStringRef(sentence, m_problems.introLatin.base);
        const std::string& transView = chooseStringRef(sentence, m_problems.introLatin.check);
        if (containsLatin(transView) && !containsLatin(origText)) {
            problemList.push_back("引入拉丁字母");
        }
    }

    if (m_problems.introHangul.use) {
        const std::string& origText = chooseStringRef(sentence, m_problems.introHangul.base);
        const std::string& transView = chooseStringRef(sentence, m_problems.introHangul.check);
        if (containsHangul(transView) && !containsHangul(origText)) {
            problemList.push_back("引入韩文");
        }
    }

    // 4. 换行符不匹配
    if ((m_problems.linebreakLost.use || m_problems.linebreakAdded.use) && !sentence->originalLinebreak.empty()) {
        const std::string& origText = chooseStringRef(sentence, m_problems.linebreakLost.base);
        const std::string& transView = chooseStringRef(sentence, m_problems.linebreakLost.check);
        int orgLinebreaks = countSubstring(origText, sentence->originalLinebreak);
        int transLinebreaks = countSubstring(transView, sentence->originalLinebreak);
        if (m_problems.linebreakLost.use && orgLinebreaks > transLinebreaks) {
            problemList.push_back("丢失换行");
        }
        if (m_problems.linebreakAdded.use && orgLinebreaks < transLinebreaks) {
            problemList.push_back("多加换行");
        }
    }

    // 5. 译文长度异常
    if (m_problems.longer.use || m_problems.strictlyLonger.use) {
        const std::string& origText = chooseStringRef(sentence, m_problems.longer.base);
        const std::string& transView = chooseStringRef(sentence, m_problems.longer.check);
        double lenBeta = m_problems.strictlyLonger.use ? 1.0 : 1.3;
        if (transView.length() > origText.length() * lenBeta && !origText.empty()) {
            double ratio = transView.length() / (double)origText.length();
            problemList.push_back(std::format("比日文长 {:.2f} 倍", ratio));
        }
    }

    // 6. 字典未使用
    if (m_problems.dictUnused.use) {
        std::string dictProblem = gptDict.checkDicUse(sentence, m_problems.dictUnused.base, m_problems.dictUnused.check);
        if (!dictProblem.empty()) {
            problemList.push_back(dictProblem);
        }
    }

    // 7. 语言不通
    if (m_problems.notTargetLang.use) {
        const std::string& origText = chooseStringRef(sentence, m_problems.notTargetLang.base);
        const std::string& transView = chooseStringRef(sentence, m_problems.notTargetLang.check);
        std::string simplifiedTargetLang = targetLang;
        if (targetLang.find('-') != std::string::npos) {
            simplifiedTargetLang = targetLang.substr(0, targetLang.find('-'));
        }
        std::string origTextToCheck = removePunctuation(origText);
        std::string transTextToCheck = removePunctuation(transView);

        size_t origTextLen = origTextToCheck.length();
        size_t transTextLen = transTextToCheck.length();

        if (origTextLen > 6 || transTextLen > 6) {
            std::set<std::string> langSet;
            auto m_langIdentifier = std::make_unique<chrome_lang_id::NNetLanguageIdentifier>(3, 300);
            if (origTextLen > 6) {
                auto results = m_langIdentifier->FindTopNMostFreqLangs(origTextToCheck, 3);
                for (const auto& result : results) {
                    if (result.language == chrome_lang_id::NNetLanguageIdentifier::kUnknown) {
                        break;
                    }
                    m_logger->trace("CLD3: {} -> {} ({}, {}, {})", origText, result.language, result.is_reliable, result.probability, result.proportion);
                    if (result.probability < m_probabilityThreshold) {
                        continue;
                    }
                    langSet.insert(result.language);
                }
            }
            if (transTextLen > 6) {
                auto results = m_langIdentifier->FindTopNMostFreqLangs(transTextToCheck, 3);
                if (results[0].language == chrome_lang_id::NNetLanguageIdentifier::kUnknown && !langSet.empty()) {
                    problemList.push_back("无法识别的语言");
                }
                for (const auto& result : results) {
                    if (result.language == chrome_lang_id::NNetLanguageIdentifier::kUnknown) {
                        break;
                    }
                    m_logger->trace("CLD3: {} -> {} ({}, {}, {})", transView, result.language, result.is_reliable, result.probability, result.proportion);
                    if (result.probability < m_probabilityThreshold) {
                        continue;
                    }
                    if (result.language != simplifiedTargetLang && langSet.find(result.language) == langSet.end()) {
                        problemList.push_back(std::format("引入({}, {:.3f})", result.language, result.probability));
                    }
                }
            }
        }
        
    }

    // 组合所有问题
    if (!problemList.empty()) {
        for (size_t i = 0; i < problemList.size(); ++i) {
            sentence->problem += problemList[i];
            if (i < problemList.size() - 1) {
                sentence->problem += ", ";
            }
        }
    }
}

void ProblemAnalyzer::loadProblems(const std::vector<std::string>& problemList, const std::string& punctSet, double langProbability)
{
    m_probabilityThreshold = langProbability;
    if (!punctSet.empty()) {
        m_checks = splitIntoGraphemes(punctSet);
    }
	for (const auto& problem : problemList)
	{
		if (problem == "词频过高") {
			m_problems.highFrequency.use = true;
		}
		else if (problem == "标点错漏") {
			m_problems.punctMiss.use = true;
		}
		else if (problem == "残留日文") {
			m_problems.remainJp.use = true;
		}
        else if (problem == "引入拉丁字母") {
            m_problems.introLatin.use = true;
        }
        else if (problem == "引入韩文") {
            m_problems.introHangul.use = true;
		}
		else if (problem == "丢失换行") {
			m_problems.linebreakLost.use = true;
		}
		else if (problem == "多加换行") {
			m_problems.linebreakAdded.use = true;
		}
		else if (problem == "比原文长") {
			m_problems.longer.use = true;
		}
		else if (problem == "比原文长严格") {
			m_problems.strictlyLonger.use = true;
		}
		else if (problem == "字典未使用") {
			m_problems.dictUnused.use = true;
		}
		else if (problem == "语言不通") {
			m_problems.notTargetLang.use = true;
		}
		else {
			throw std::invalid_argument("未知问题: " + problem);
		}
	}
}

void ProblemAnalyzer::overwriteCompareObj(const std::string& problemKey, const std::string& base, const std::string& check)
{
    auto saveCachePart = [](ProblemCompareObj& obj, const std::string& base, const std::string& check)
        {
            obj.base = chooseCachePart(base);
            obj.check = chooseCachePart(check);
            if (obj.base == CachePart::None) {
                throw std::invalid_argument("未知缓存键: " + base);
            }
            if (obj.check == CachePart::None) {
                throw std::invalid_argument("未知缓存键: " + check);
            }
        };

    if (problemKey == "词频过高") {
        saveCachePart(m_problems.highFrequency, base, check);
    }
    else if (problemKey == "标点错漏") {
        saveCachePart(m_problems.punctMiss, base, check);
    }
    else if (problemKey == "残留日文") {
        saveCachePart(m_problems.remainJp, base, check);
    }
    else if (problemKey == "引入拉丁字母") {
        saveCachePart(m_problems.introLatin, base, check);
    }
    else if (problemKey == "引入韩文") {
        saveCachePart(m_problems.introHangul, base, check);
    }
    else if (problemKey == "丢失换行") {
        saveCachePart(m_problems.linebreakLost, base, check);
    }
    else if (problemKey == "多加换行") {
        saveCachePart(m_problems.linebreakAdded, base, check);
    }
    else if (problemKey == "比原文长") {
        saveCachePart(m_problems.longer, base, check);
    }
    else if (problemKey == "比原文长严格") {
        saveCachePart(m_problems.strictlyLonger, base, check);
    }
    else if (problemKey == "字典未使用") {
        saveCachePart(m_problems.dictUnused, base, check);
    }
    else if (problemKey == "语言不通") {
        saveCachePart(m_problems.notTargetLang, base, check);
    }
    else {
        throw std::invalid_argument("不支持的问题比较对象覆写: " + problemKey);
    }
}
