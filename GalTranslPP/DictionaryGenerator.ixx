module;

#include <mecab/mecab.h>
#include <spdlog/spdlog.h>
#include <boost/regex.hpp>
#include <cpr/cpr.h>
#include <ranges>

export module DictionaryGenerator;

import <nlohmann/json.hpp>;
import <ctpl_stl.h>;
import <toml.hpp>;
import APIPool;
import Dictionary;

namespace fs = std::filesystem;
using json = nlohmann::json;

export {
    class DictionaryGenerator {
    private:
        APIPool& m_apiPool;
        NormalDictionary& m_preDict;
        std::shared_ptr<IController> m_controller;
        std::string m_systemPrompt;
        std::string m_userPrompt;
        int m_threadsNum;
        int m_apiTimeoutMs;
        std::shared_ptr<spdlog::logger> m_logger;
        std::string m_apiStrategy;
        int m_maxRetries;
        bool m_checkQuota;
        bool m_usePreDictInName;
        bool m_usePreDictInMsg;

        // 阶段一和二的结果
        std::vector<std::string> m_segments;
        std::vector<std::set<std::string>> m_segmentWords;
        std::map<std::string, int> m_wordCounter;
        std::set<std::string> m_nameSet;

        // 阶段四的结果 (线程安全)
        std::vector<std::tuple<std::string, std::string, std::string>> m_finalDict;
        std::map<std::string, int> m_finalCounter;
        std::mutex m_resultMutex;

        // MeCab 解析器
        std::unique_ptr<MeCab::Tagger> m_tagger;

        void preprocessAndTokenize(const std::vector<fs::path>& jsonFiles);
        std::vector<int> solveSentenceSelection();
        void callLLMToGenerate(int segmentIndex, int threadId);

    public:
        DictionaryGenerator(std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger, APIPool& apiPool,
            NormalDictionary& preDict, const fs::path& dictDir, const std::string& systemPrompt, const std::string& userPrompt, const std::string& apiStrategy,
            int maxRetries, int threadsNum, int apiTimeoutMs, bool checkQuota, bool usePreDictInName, bool usePreDictInMsg);
        void generate(const std::vector<fs::path>& jsonFiles, const fs::path& outputFilePath);
    };
}


module :private;

DictionaryGenerator::DictionaryGenerator(std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger, APIPool& apiPool,
    NormalDictionary& preDict, const fs::path& dictDir, const std::string& systemPrompt, const std::string& userPrompt, const std::string& apiStrategy,
    int maxRetries, int threadsNum, int apiTimeoutMs, bool checkQuota, bool usePreDictInName, bool usePreDictInMsg)
    : m_controller(controller), m_logger(logger), m_apiPool(apiPool), m_preDict(preDict), m_systemPrompt(systemPrompt), m_userPrompt(userPrompt),
    m_apiStrategy(apiStrategy), m_maxRetries(maxRetries),
    m_threadsNum(threadsNum), m_apiTimeoutMs(apiTimeoutMs), m_checkQuota(checkQuota), m_usePreDictInName(usePreDictInName), m_usePreDictInMsg(usePreDictInMsg)
{
    m_tagger.reset(
        MeCab::createTagger(("-r BaseConfig/DictGenerator/mecabrc -d " + wide2Ascii(dictDir, 0)).c_str())
    );
    if (!m_tagger) {
        throw std::runtime_error("无法初始化 MeCab Tagger。请确保 BaseConfig/DictGenerator/mecabrc 和 " + wide2Ascii(dictDir) + " 存在且无特殊字符");
    }
}

void DictionaryGenerator::preprocessAndTokenize(const std::vector<fs::path>& jsonFiles) {
    m_logger->info("阶段一：预处理和分词...");
    std::string currentSegment;
    const size_t MAX_SEGMENT_LEN = 512;

    Sentence se;
    std::ifstream ifs;
    for (const auto& item : jsonFiles
        | std::views::transform([&](const fs::path& filePath)
            {
                ifs.open(filePath);
                json data = json::parse(ifs);
                ifs.close();
                return data;
            }) | std::views::join)
    {
        if (item.contains("name")) {
            se.nameType = NameType::Single;
            se.name = item.value("name", "");
            if (m_usePreDictInName) {
                se.name = m_preDict.doReplace(&se, CachePart::Name);
            }
        }
        else if (item.contains("names")) {
            se.nameType = NameType::Multiple;
            se.names = item["names"].get<std::vector<std::string>>();
            if (m_usePreDictInName) {
                for (auto& name : se.names) {
                    se.name = name;
                    name = m_preDict.doReplace(&se, CachePart::Name);
                }
            }
        }
        if (se.nameType == NameType::Single && !se.name.empty()) {
            m_nameSet.insert(se.name);
            m_wordCounter[se.name] += 2;
        }
        else if (se.nameType == NameType::Multiple) {
            for (const auto& name : se.names | std::views::filter([](const std::string& name) { return !name.empty(); })) {
                m_nameSet.insert(name);
                m_wordCounter[name] += 2;
            }
        }

        se.original_text = item.value("message", "");
        if (m_usePreDictInMsg) {
            se.pre_processed_text = m_preDict.doReplace(&se, CachePart::OrigText);
        }

        currentSegment += getNameString(&se) + se.pre_processed_text + "\n";
        if (currentSegment.length() > MAX_SEGMENT_LEN) {
            m_segments.push_back(currentSegment);
            currentSegment.clear();
        }
    }
    
    if (!currentSegment.empty()) {
        m_segments.push_back(currentSegment);
    }

    m_logger->info("共分割成 {} 个文本块，开始使用 MeCab 分词...", m_segments.size());
    m_segmentWords.reserve(m_segments.size());
    for (const auto& segment : m_segments) {
        std::set<std::string> wordsInSegment;
        const MeCab::Node* node = m_tagger->parseToNode(segment.c_str());
        for (; node; node = node->next) {
            if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) continue;

            std::string surface(node->surface, node->length);
            std::string feature = node->feature;

            m_logger->trace("分词结果：{} ({})", surface, feature);

            if (surface.length() <= 1) continue;

            if (feature.find("固有名詞") != std::string::npos || !extractKatakana(surface).empty()) {
                wordsInSegment.insert(surface);
                m_wordCounter[surface]++;
            }
        }
        m_segmentWords.push_back(wordsInSegment);
    }
}

std::vector<int> DictionaryGenerator::solveSentenceSelection() {
    m_logger->info("阶段二：搜索并选择信息量最大的文本块...");

    // 剔除出现次数小于2的词语，人名除外
    std::set<std::string> allWords;
    for (const auto& [word, count] : m_wordCounter) {
        if (count >= 2 || m_nameSet.count(word)) {
            allWords.insert(word);
        }
    }

    // 过滤每个 segment 中的词，只保留在 allWords 中的
    std::vector<std::set<std::string>> filteredSegmentWords;
    for (const auto& segment : m_segmentWords) {
        std::set<std::string> filteredSet;
        std::set_intersection(segment.begin(), segment.end(),
            allWords.begin(), allWords.end(),
            std::inserter(filteredSet, filteredSet.begin()));
        filteredSegmentWords.push_back(filteredSet);
    }

    std::set<std::string> coveredWords;
    std::vector<int> selectedIndices;
    std::vector<int> remainingIndices(filteredSegmentWords.size());
    std::iota(remainingIndices.begin(), remainingIndices.end(), 0);

    while (coveredWords.size() < allWords.size() && !remainingIndices.empty()) {
        int bestIndex = -1;
        size_t maxNewCoverage = 0;

        for (int index : remainingIndices) {
            std::set<std::string> tempSet;
            std::set_difference(
                filteredSegmentWords[index].begin(), filteredSegmentWords[index].end(),
                coveredWords.begin(), coveredWords.end(),
                std::inserter(tempSet, tempSet.begin())
            );
            size_t newCoverage = tempSet.size();

            if (newCoverage > maxNewCoverage) {
                maxNewCoverage = newCoverage;
                bestIndex = index;
            }
        }

        if (bestIndex != -1) {
            coveredWords.insert(filteredSegmentWords[bestIndex].begin(), filteredSegmentWords[bestIndex].end());
            selectedIndices.push_back(bestIndex);
            remainingIndices.erase(std::remove(remainingIndices.begin(), remainingIndices.end(), bestIndex), remainingIndices.end());
        }
        else {
            break;
        }
    }
    return selectedIndices;
}

void DictionaryGenerator::callLLMToGenerate(int segmentIndex, int threadId) {
    if (m_controller->shouldStop()) {
        return;
    }
    m_controller->addThreadNum();
    std::string text = m_segments[segmentIndex];
    std::string hint = "无";
    std::string nameHit;
    for (const auto& name : m_nameSet) {
        if (text.find(name) != std::string::npos) {
            nameHit += name + "\n";
        }
    }
    if (!nameHit.empty()) {
        hint = "输入文本中的这些词语是一定要加入术语表的: \n" + nameHit;
    }

    std::string prompt = m_userPrompt;
    replaceStrInplace(prompt, "{input}", text);
    replaceStrInplace(prompt, "{hint}", hint);

    json messages = json::array({
        {{"role", "system"}, {"content", m_systemPrompt}},
        {{"role", "user"}, {"content", prompt}}
        });

    int retryCount = 0;
    while (retryCount < m_maxRetries) {
        if (m_controller->shouldStop()) {
            return;
        }
        auto optAPI = m_apiStrategy == "random" ? m_apiPool.getAPI() : m_apiPool.getFirstAPI();
        if (!optAPI) {
            throw std::runtime_error("没有可用的API Key了");
        }
        TranslationAPI currentAPI = optAPI.value();
        json payload = { {"model", currentAPI.modelName}, {"messages", messages}, /*{"temperature", 0.6}*/ };

        m_logger->info("[线程 {}] 开始从段落中生成术语表\ninputBlock: \n{}", threadId, text);
        ApiResponse response = performApiRequest(payload, currentAPI, threadId, m_controller, m_logger, m_apiTimeoutMs);

        /*bool checkResponse(const ApiResponse & response, const TranslationAPI & currentAPI, int& retryCount, const std::filesystem::path & relInputPath,
            int threadId, bool m_checkQuota, const std::string & m_apiStrategy, APIPool & m_apiPool, std::shared_ptr<spdlog::logger> m_logger);*/
        if (!checkResponse(
            response, currentAPI, retryCount, L"字典生成——段落输入", threadId, m_checkQuota, m_apiStrategy, m_apiPool, m_logger
        )) {
            continue;
        }
        else {
            m_logger->info("[线程 {}] AI 字典生成成功:\n {}", threadId, response.content);
            auto lines = splitString(response.content, '\n');
            for (const auto& line : lines) {
                auto parts = splitString(line, '\t');
                if (parts.size() < 3 || parts[0].starts_with("日文原词") || parts[0].starts_with("NULL")) continue;

                std::lock_guard<std::mutex> lock(m_resultMutex);
                m_finalCounter[parts[0]]++;
                if (m_finalCounter[parts[0]] == 2) {
                    m_logger->trace("发现重复术语: {}\t{}\t{}", parts[0], parts[1], parts[2]);
                }
                m_finalDict.emplace_back(parts[0], parts[1], parts[2]);
            }
            break;
        }
    }
    if (retryCount >= m_maxRetries) {
        m_logger->error("[线程 {}] AI 字典生成失败，已达到最大重试次数。", threadId);
    }

    m_controller->updateBar();
    m_controller->reduceThreadNum();
}

void DictionaryGenerator::generate(const std::vector<fs::path>& jsonFiles, const fs::path& outputFilePath) {

    preprocessAndTokenize(jsonFiles);
    auto selectedIndices = solveSentenceSelection();
    if (selectedIndices.size() > 128) {
        selectedIndices.resize(128);
    }

    int threadsNum = std::min(m_threadsNum, (int)selectedIndices.size());
    m_logger->info("阶段三：启动 {} 个线程，向 AI 发送 {} 个任务...", threadsNum, selectedIndices.size());
    m_controller->makeBar((int)selectedIndices.size(), threadsNum);
    ctpl::thread_pool pool(threadsNum);
    std::vector<std::future<void>> results;
    for (int segmentIdx : selectedIndices) {
        results.emplace_back(pool.push([=](int threadId) 
            {
                this->callLLMToGenerate(segmentIdx, threadId);
            }));
    }
    for (auto& res : results) {
        res.get();
    }

    m_logger->info("阶段四：整理并保存结果...");
    std::vector<std::tuple<std::string, std::string, std::string>> finalList;

    // 按出现次数排序
    std::sort(m_finalDict.begin(), m_finalDict.end(), [&](const auto& a, const auto& b) 
        {
            return m_finalCounter[std::get<0>(a)] > m_finalCounter[std::get<0>(b)];
        });

    // 过滤
    for (const auto& item : m_finalDict) {
        const auto& src = std::get<0>(item);
        const auto& note = std::get<2>(item);
        if (m_finalCounter[src] > 1 || note.find("人名") != std::string::npos || note.find("地名") != std::string::npos || m_wordCounter.count(src) || m_nameSet.count(src)) {
            finalList.push_back(item);
        }
    }

    // 去重
    std::set<std::string> seen;
    finalList.erase(std::remove_if(finalList.begin(), finalList.end(), [&](const auto& item) 
        {
            if (seen.count(std::get<0>(item))) {
                return true;
            }
            seen.insert(std::get<0>(item));
            return false;
        }), finalList.end());


    toml::ordered_value arr = toml::array{};
    
    for (const auto& item : finalList) {
        arr.push_back(toml::ordered_table{ { "org", std::get<0>(item) }, { "rep", std::get<1>(item) }, { "note", std::get<2>(item) } });
    }
    
    arr.as_array_fmt().fmt = toml::array_format::multiline;
    std::ofstream ofs(outputFilePath);
    ofs << toml::format(toml::ordered_value{ toml::ordered_table{{ "gptDict", arr }} });
    m_logger->info("字典生成完成，共 {} 个词语，已保存到 {}", finalList.size(), wide2Ascii(outputFilePath));
}
