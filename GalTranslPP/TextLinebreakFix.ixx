module;

#include <spdlog/spdlog.h>

export module TextLinebreakFix;

import <toml.hpp>;
import Tool;
export import IPlugin;

namespace fs = std::filesystem;

export {
	class TextLinebreakFix : public IPlugin {

	private:

		std::string m_mode;
		int m_segmentThreshold;
		int m_errorThreshold;
		bool m_forceFix;
		bool m_onlyAddAfterPunct;

	public:

		TextLinebreakFix(const fs::path& projectDir, std::shared_ptr<spdlog::logger> logger);

		virtual void run(Sentence* se) override;

		virtual ~TextLinebreakFix() = default;
	};
}


module :private;

TextLinebreakFix::TextLinebreakFix(const fs::path& projectDir, std::shared_ptr<spdlog::logger> logger)
	: IPlugin(projectDir, logger)
{
	try {
		const auto projectConfig = toml::parse(m_projectDir / L"config.toml");
		const auto pluginConfig = toml::parse(pluginConfigsPath / L"textPostPlugins/TextLinebreakFix.toml");

		m_mode = parseToml<std::string>(projectConfig, pluginConfig, "plugins.TextLinebreakFix.换行模式");
		m_onlyAddAfterPunct = parseToml<bool>(projectConfig, pluginConfig, "plugins.TextLinebreakFix.仅在标点后添加");
		m_segmentThreshold = parseToml<int>(projectConfig, pluginConfig, "plugins.TextLinebreakFix.分段字数阈值");
		m_forceFix = parseToml<bool>(projectConfig, pluginConfig, "plugins.TextLinebreakFix.强制修复");
		m_errorThreshold = parseToml<int>(projectConfig, pluginConfig, "plugins.TextLinebreakFix.报错阈值");

		if (m_segmentThreshold <= 0) {
			throw std::runtime_error("分段字数阈值必须大于0");
		}
		if (m_errorThreshold <= 0) {
			throw std::runtime_error("报错阈值必须大于0");
		}

		m_logger->info("已加载插件 TextLinebreakFix, 换行模式: {}, 仅在标点后添加 {}, 分段字数阈值: {}, 强制修复: {}, 报错阈值: {}",
			m_mode, m_onlyAddAfterPunct, m_segmentThreshold, m_forceFix, m_errorThreshold);
	}
	catch (const toml::exception& e) {
		m_logger->critical("换行修复配置文件解析错误");
		throw std::runtime_error(e.what());
	}
}

void TextLinebreakFix::run(Sentence* se)
{
	int origLinebreakCount = countSubstring(se->pre_processed_text, "<br>");
	int transLinebreakCount = countSubstring(se->translated_preview, "<br>");

	if (transLinebreakCount == origLinebreakCount && !m_forceFix) {
		return;
	}

	m_logger->debug("需要修复换行的句子[{}]: 原文 {} 行, 译文 {} 行", se->pre_processed_text, origLinebreakCount + 1, transLinebreakCount + 1);

	std::string orgTransPreview = se->translated_preview;
	std::string transViewToModify = se->translated_preview;
	replaceStrInplace(transViewToModify, "<br>", "");
	if (transViewToModify.empty()) {
		for (int i = 0; i < origLinebreakCount; i++) {
			transViewToModify += "<br>";
		}
		se->translated_preview = transViewToModify;

		se->other_info["换行修复"] = std::format("原文 {} 行, 译文 {} 行, 修正后 {} 行", origLinebreakCount + 1, transLinebreakCount + 1, transLinebreakCount + 1);
		m_logger->debug("译文[{}]({}行): 修正后译文[{}]({}行)", orgTransPreview, origLinebreakCount + 1, se->translated_preview, transLinebreakCount + 1);
		return;
	}
	std::vector<std::string> graphemes = splitIntoGraphemes(transViewToModify);

	if (m_mode == "平均") {
		int linebreakAdded = 0;

		size_t charCountLine = graphemes.size() / (origLinebreakCount + 1);
		if (charCountLine == 0) {
			charCountLine = 1;
		}

		transViewToModify.clear();
		for (size_t i = 0; i < graphemes.size(); i++) {
			transViewToModify += graphemes[i];
			if ((i + 1) % charCountLine == 0 && linebreakAdded < origLinebreakCount) {
				transViewToModify += "<br>";
				linebreakAdded++;
			}
		}
	}
	else if (m_mode == "固定字数") {

		transViewToModify.clear();
		for (size_t i = 0; i < graphemes.size(); i++) {
			transViewToModify += graphemes[i];
			if ((i + 1) % m_segmentThreshold == 0 && i != graphemes.size() - 1) {
				transViewToModify += "<br>";
			}
		}
	}
	else if (m_mode == "保持位置") {
		std::vector<double> relPositons = getSubstringPositions(se->pre_processed_text, "<br>");

		std::vector<size_t> linebreakPositions;
		for (double pos : relPositons) {
			linebreakPositions.push_back(static_cast<size_t>(pos * transViewToModify.length()));
		}

		std::reverse(graphemes.begin(), graphemes.end());

		transViewToModify.clear();

		size_t currentPos = 0;
		for (size_t pos : linebreakPositions) {
			while (currentPos < pos) {
				if (graphemes.empty()) {
					break;
				}
				transViewToModify += graphemes.back();
				currentPos += graphemes.back().length();
				graphemes.pop_back();
			}
			transViewToModify += "<br>";
		}

		while (!graphemes.empty()) {
			transViewToModify += graphemes.back();
			graphemes.pop_back();
		}
	}
	else if (m_mode == "优先标点") {

		std::vector<double> relPositons = getSubstringPositions(se->pre_processed_text, "<br>");

		std::vector<std::pair<size_t, size_t>> punctPositions;
		size_t currentPos = 0;
		for (size_t i = 0; i < graphemes.size(); i++) {
			currentPos += graphemes[i].length();
			if (removePunctuation(graphemes[i]).empty()) {
				punctPositions.push_back(std::make_pair(currentPos - graphemes[i].length(), currentPos));
			}
		}
		std::set<std::string> excludePunct =
		{ "『", "「", "“", "‘", "'", "《", "〈", "（", "【", "〔", "〖" };
		std::erase_if(punctPositions, [&](auto& pos)
			{
				if (pos.second >= transViewToModify.length()) {
					return true;
				}
				std::string punctStr = transViewToModify.substr(pos.first, pos.second - pos.first);
				// 不在这些标点后添加换行符
				if (excludePunct.contains(punctStr)) {
					return true;
				}
				// 如果标点后面还有标点，则不插入换行符
				return std::any_of(punctPositions.begin(), punctPositions.end(), [&](auto& otherPos)
					{
						return pos.second == otherPos.first;
					});
			});

		std::vector<size_t> linebreakPositions; // 根据相对位置大致估算新换行在译文中的位置
		for (double pos : relPositons) {
			linebreakPositions.push_back(static_cast<size_t>(pos * transViewToModify.length()));
		}

		std::vector<size_t> positionsToAddLinebreak; // 最终要在 transViewToModify 中插入换行符的位置

		if (punctPositions.size() == origLinebreakCount) {
			for (auto& pos : punctPositions) {
				positionsToAddLinebreak.push_back(pos.second);
			}
		}
		else if (punctPositions.size() < origLinebreakCount) {
			for (auto& pos : punctPositions) {
				positionsToAddLinebreak.push_back(pos.second);
			}
			if (!m_onlyAddAfterPunct) {
				for (size_t pos : positionsToAddLinebreak) {
					auto nearestIterator = std::min_element(linebreakPositions.begin(), linebreakPositions.end(), [&](size_t a, size_t b)
						{
							return calculateAbs(a, pos) < calculateAbs(b, pos);
						});
					linebreakPositions.erase(nearestIterator);
				}
				for (size_t pos : linebreakPositions) {
					size_t currentPos = 0;
					for (size_t i = 0; i < graphemes.size(); i++) {
						currentPos += graphemes[i].length();
						if (currentPos >= pos) {
							break;
						}
					}
					if (currentPos < transViewToModify.length() && std::find(positionsToAddLinebreak.begin(), positionsToAddLinebreak.end(), currentPos) == positionsToAddLinebreak.end()) {
						positionsToAddLinebreak.push_back(currentPos);
					}
				}
			}
		}
		else {
			for (size_t pos : linebreakPositions) {
				auto nearestIterator = std::min_element(punctPositions.begin(), punctPositions.end(), [&](auto& a, auto& b)
					{
						return calculateAbs(a.second, pos) < calculateAbs(b.second, pos);
					});
				positionsToAddLinebreak.push_back(nearestIterator->second);
				punctPositions.erase(nearestIterator);
			}
		}

		std::ranges::sort(positionsToAddLinebreak, [&](size_t a, size_t b)
			{
				return a > b;
			});

		for (size_t pos : positionsToAddLinebreak) {
			transViewToModify.insert(pos, "<br>");
		}

	}
	else {
		throw std::runtime_error("未知的换行模式: " + m_mode);
	}

	se->translated_preview = transViewToModify;

	if (transViewToModify.length() > m_errorThreshold) {
		std::vector<std::string> newLines = splitString(transViewToModify, "<br>");
		size_t lineIndex = 0;
		for (const std::string& newLine : newLines) {
			lineIndex++;
			if (size_t charCount = splitIntoGraphemes(newLine).size(); charCount > m_errorThreshold) {
				m_logger->error("句子[{}](原有{}行)其中的 译文行[{}]超出报错阈值[{}/{}]", se->pre_processed_text, origLinebreakCount + 1, newLine, charCount, m_errorThreshold);
				se->problems.push_back(std::format("第 {} 行超出报错阈值[{}/{}]", lineIndex, charCount, m_errorThreshold));
			}
		}
	}

	int newLinebreakCount = countSubstring(se->translated_preview, "<br>");
	se->other_info["换行修复"] = std::format("原文 {} 行, 译文 {} 行, 修正后 {} 行", origLinebreakCount + 1, transLinebreakCount + 1, newLinebreakCount + 1);
	m_logger->debug("句子[{}]({}行): 修正后译文[{}]({}行)", orgTransPreview, transLinebreakCount + 1, se->translated_preview, newLinebreakCount + 1);
}

