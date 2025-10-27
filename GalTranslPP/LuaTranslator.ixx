module;

#include <spdlog/spdlog.h>
#include <sol/sol.hpp>
#define PATH_CVT(className, memberName) sol::property([](className& self) { return wide2Ascii(self.memberName); }, \
[](className& self, const std::string& pathStr) { self.memberName = ascii2Wide(pathStr); })

export module LuaTranslator;

import Tool;
import NormalJsonTranslator;
import EpubTranslator;
import PDFTranslator;
import LuaManager;

namespace fs = std::filesystem;

export {

	struct JsonStrInfo {
		std::vector<EpubTextNodeInfo> metadata;
		// 存储json文件相对路径到原始 HTML 完整路径的映射
		std::string htmlPath;
		// 存储json文件相对路径到其所属 epub 完整路径的映射
		std::string epubPath;
		// 存储json文件相对路径到 normal_post 完整路径的映射
		std::string normalPostPath;
	};

	template<typename Base>
	class LuaTranslator : public Base {
	private:
		std::shared_ptr<LuaStateInstance> m_luaState;
		sol::function m_luaRunFunc;
		std::string m_scriptPath;
		std::string m_translatorName;
		bool m_needReboot = false;

	public:
		virtual void run() override
		{
			this->m_logger->info("开始运行 LuaTranslator。");
			try {
				m_luaRunFunc();
			}
			catch (const sol::error& e) {
				throw std::runtime_error(std::string("运行 LuaTranslator 失败: ") + e.what());
			}
		}

		template <typename... Args>
		LuaTranslator(const std::string& scriptPath, Args&&... baseArgs) :
			Base(std::forward<Args>(baseArgs)...), m_scriptPath(scriptPath)
		{
			m_translatorName = wide2Ascii(fs::path(ascii2Wide(m_scriptPath)).filename());
			// m_inputDir = L"cache" / projectDir.filename() / (ascii2Wide(m_translatorName) + L"_json_input");
			// m_outputDir = L"cache" / projectDir.filename() / (ascii2Wide(m_translatorName) + L"_json_output");
			std::optional<std::shared_ptr<LuaStateInstance>> luaStateOpt = this->m_luaManager.registerFunction(scriptPath, "init", m_needReboot);
			if (!luaStateOpt.has_value()) {
				throw std::runtime_error("LuaTranslator 获取 init 函数失败。");
			}
			luaStateOpt = this->m_luaManager.registerFunction(scriptPath, "run", m_needReboot);
			if (!luaStateOpt.has_value()) {
				throw std::runtime_error("LuaTranslator 获取 run 函数失败。");
			}
			m_luaState = luaStateOpt.value();
			m_luaRunFunc = m_luaState->functions["run"];
			this->m_luaManager.registerFunction(scriptPath, "unload", m_needReboot);

			sol::state& luaState = *(m_luaState->lua);
			if constexpr (std::is_base_of_v<ITranslator, Base>) {
				luaState.new_usertype<ITranslator>("ITranslator",
					sol::no_constructor,
					"run", &ITranslator::run
				);
			}
			if constexpr (std::is_base_of_v<NormalJsonTranslator, Base>) {
				luaState.new_usertype<IController>("IController",
					"make_bar", &IController::makeBar,
					"write_log", &IController::writeLog,
					"add_thread_num", &IController::addThreadNum,
					"reduce_thread_num", &IController::reduceThreadNum,
					"update_bar", &IController::updateBar,
					"should_stop", &IController::shouldStop,
					"flush", &IController::flush
				);
				luaState.new_usertype<NormalJsonTranslator>("NormalJsonTranslator",
					sol::base_classes, sol::bases<ITranslator>(),
					"m_transEngine", &NormalJsonTranslator::m_transEngine,
					"m_controller", &NormalJsonTranslator::m_controller,
					"m_projectDir", PATH_CVT(NormalJsonTranslator, m_projectDir),
					"m_inputDir", PATH_CVT(NormalJsonTranslator, m_inputDir),
					"m_inputCacheDir", PATH_CVT(NormalJsonTranslator, m_inputCacheDir),
					"m_outputDir", PATH_CVT(NormalJsonTranslator, m_outputDir),
					"m_outputCacheDir", PATH_CVT(NormalJsonTranslator, m_outputCacheDir),
					"m_cacheDir", PATH_CVT(NormalJsonTranslator, m_cacheDir),
					"m_systemPrompt", &NormalJsonTranslator::m_systemPrompt,
					"m_userPrompt", &NormalJsonTranslator::m_userPrompt,
					"m_targetLang", &NormalJsonTranslator::m_targetLang,
					"m_totalSentences", &NormalJsonTranslator::m_totalSentences,
					"m_completedSentences", &NormalJsonTranslator::m_completedSentences,
					"m_threadsNum", &NormalJsonTranslator::m_threadsNum,
					"m_batchSize", &NormalJsonTranslator::m_batchSize,
					"m_contextHistorySize", &NormalJsonTranslator::m_contextHistorySize,
					"m_maxRetries", &NormalJsonTranslator::m_maxRetries,
					"m_saveCacheInterval", &NormalJsonTranslator::m_saveCacheInterval,
					"m_apiTimeOutMs", &NormalJsonTranslator::m_apiTimeOutMs,
					"m_checkQuota", &NormalJsonTranslator::m_checkQuota,
					"m_smartRetry", &NormalJsonTranslator::m_smartRetry,
					"m_usePreDictInName", &NormalJsonTranslator::m_usePreDictInName,
					"m_usePostDictInName", &NormalJsonTranslator::m_usePostDictInName,
					"m_usePreDictInMsg", &NormalJsonTranslator::m_usePreDictInMsg,
					"m_usePostDictInMsg", &NormalJsonTranslator::m_usePostDictInMsg,
					"m_useGptDictToReplaceName", &NormalJsonTranslator::m_useGptDictToReplaceName,
					"m_outputWithSrc", &NormalJsonTranslator::m_outputWithSrc,
					"m_apiStrategy", &NormalJsonTranslator::m_apiStrategy,
					"m_sortMethod", &NormalJsonTranslator::m_sortMethod,
					"m_splitFile", &NormalJsonTranslator::m_splitFile,
					"m_splitFileNum", &NormalJsonTranslator::m_splitFileNum,
					"m_linebreakSymbol", &NormalJsonTranslator::m_linebreakSymbol,
					"m_needsCombining", &NormalJsonTranslator::m_needsCombining,
					"m_splitFilePartsToJson", sol::property([](NormalJsonTranslator& self, sol::this_state s)
						{
							sol::state_view lua(s);
							sol::table partsTable = lua.create_table();
							for (const auto& [key, value] : self.m_splitFilePartsToJson) {
								partsTable[wide2Ascii(key)] = wide2Ascii(value);
							}
							return partsTable;
						},
						[](NormalJsonTranslator& self, sol::table partsTable)
						{
							self.m_splitFilePartsToJson.clear();
							for (const auto& [key, value] : partsTable) {
								if (key.is<std::string>() && value.is<std::string>()) {
									self.m_splitFilePartsToJson.insert({ ascii2Wide(key.as<std::string>()), ascii2Wide(value.as<std::string>()) });
								}
							}
						}),
					"m_jsonToSplitFileParts", sol::property([](NormalJsonTranslator& self, sol::this_state s)
						{
							sol::state_view lua(s);
							sol::table partsTable = lua.create_table();
							for (const auto& [key, value] : self.m_jsonToSplitFileParts) {
								sol::table partTable = lua.create_table();
								for (const auto& [part, isComplete] : value) {
									partTable[wide2Ascii(part)] = isComplete;
								}
								partsTable[wide2Ascii(key)] = partTable;
							}
							return partsTable;
						},
						[](NormalJsonTranslator& self, sol::table partsTable)
						{
							self.m_jsonToSplitFileParts.clear();
							for (const auto& [key, value] : partsTable) {
								if (key.is<std::string>() && value.is<sol::table>()) {
									std::map<fs::path, bool> partMap;
									for (const auto& [part, isComplete] : value.as<sol::table>()) {
										if (part.is<std::string>() && isComplete.is<bool>()) {
											partMap[ascii2Wide(part.as<std::string>())] = isComplete.as<bool>();
										}
									}
									self.m_jsonToSplitFileParts.insert({ ascii2Wide(key.as<std::string>()), partMap });
								}
							}
						}),
					"m_onFileProcessed", sol::property([](NormalJsonTranslator& self, sol::this_state s)
						{
							sol::state_view lua(s);
							NormalJsonTranslator* classPtr = &self;
							std::function<void(std::string)> onFileProcessed = [classPtr](std::string relFilePath)
								{
									classPtr->m_onFileProcessed(ascii2Wide(relFilePath));
								};
							return onFileProcessed;
						},
						[](NormalJsonTranslator& self, std::function<void(std::string)> onFileProcessed)
						{
							// 这里的相对路径是相对 gt_input 来说的
							self.m_onFileProcessed = [onFileProcessed](fs::path relFilePath)
								{
									onFileProcessed(wide2Ascii(relFilePath));
								};
						}),
					"preProcess", &NormalJsonTranslator::preProcess,
					"postProcess", &NormalJsonTranslator::postProcess,
					"processFile", &NormalJsonTranslator::processFile,
					"normalJsonTranslator_run", [](NormalJsonTranslator& self){ self.NormalJsonTranslator::run(); }
				);
			}

			if constexpr (std::is_base_of_v<EpubTranslator, Base>) {
				luaState.new_usertype<EpubTextNodeInfo>("EpubTextNodeInfo",
					sol::constructors<EpubTextNodeInfo()>(),
					"offset", &EpubTextNodeInfo::offset,
					"length", &EpubTextNodeInfo::length
				);
				luaState.new_usertype<JsonStrInfo>("JsonStrInfo",
					sol::constructors<JsonStrInfo()>(),
					"metadata", &JsonStrInfo::metadata,
					"htmlPath", &JsonStrInfo::htmlPath,
					"epubPath", &JsonStrInfo::epubPath,
					"normalPostPath", &JsonStrInfo::normalPostPath
				);
				luaState.new_usertype<EpubTranslator>("EpubTranslator",
					sol::base_classes, sol::bases<NormalJsonTranslator>(),
					"m_epubInputDir", PATH_CVT(EpubTranslator, m_epubInputDir),
					"m_epubOutputDir", PATH_CVT(EpubTranslator, m_epubOutputDir),
					"m_tempUnpackDir", PATH_CVT(EpubTranslator, m_tempUnpackDir),
					"m_tempRebuildDir", PATH_CVT(EpubTranslator, m_tempRebuildDir),
					"m_bilingualOutput", &EpubTranslator::m_bilingualOutput,
					"m_originalTextColor", &EpubTranslator::m_originalTextColor,
					"m_originalTextScale", &EpubTranslator::m_originalTextScale,
					"m_jsonToInfoMap", sol::property([](EpubTranslator& self, sol::this_state s)
						{
							sol::state_view lua(s);
							sol::table infoTable = lua.create_table();
							for (const auto& [key, value] : self.m_jsonToInfoMap) {
								JsonStrInfo info;
								info.metadata = value.metadata;
								info.htmlPath = wide2Ascii(value.htmlPath);
								info.epubPath = wide2Ascii(value.epubPath);
								info.normalPostPath = wide2Ascii(value.normalPostPath);
								infoTable[wide2Ascii(key)] = info;
							}
							return infoTable;
						},
						[](EpubTranslator& self, sol::table infoTable)
						{
							self.m_jsonToInfoMap.clear();
							for (const auto& kv : infoTable) {
								if (kv.first.is<std::string>()) {
									JsonInfo info;
									JsonStrInfo strInfo = kv.second.as<JsonStrInfo>();
									info.metadata = strInfo.metadata;
									info.htmlPath = ascii2Wide(strInfo.htmlPath);
									info.epubPath = ascii2Wide(strInfo.epubPath);
									info.normalPostPath = ascii2Wide(strInfo.normalPostPath);
									self.m_jsonToInfoMap[ascii2Wide(kv.first.as<std::string>())] = info;
								}
							}
						}),
					"m_epubToJsonsMap", sol::property([](EpubTranslator& self, sol::this_state s)
						{
							sol::state_view lua(s);
							sol::table infoTable = lua.create_table();
							for (const auto& [key, value] : self.m_epubToJsonsMap) {
								sol::table jsonTable = lua.create_table();
								for (const auto& [json, isComplete] : value) {
									jsonTable[wide2Ascii(json)] = isComplete;
								}
								infoTable[wide2Ascii(key)] = jsonTable;
							}
							return infoTable;
						},
						[](EpubTranslator& self, sol::table infoTable)
						{
							self.m_epubToJsonsMap.clear();
							for (const auto& kv : infoTable) {
								if (kv.first.is<std::string>()) {
									std::map<fs::path, bool> jsonMap;
									sol::table jsonTable = kv.second.as<sol::table>();
									for (const auto& [json, isComplete] : jsonTable) {
										if (json.is<std::string>() && isComplete.is<bool>()) {
											jsonMap[ascii2Wide(json.as<std::string>())] = isComplete.as<bool>();
										}
									}
									self.m_epubToJsonsMap[ascii2Wide(kv.first.as<std::string>())] = jsonMap;
								}
							}
						}),
					"epubTranslator_run", [](EpubTranslator& self)
					{
						self.EpubTranslator::run();
					}
				);
			}

			if constexpr (std::is_base_of_v<PDFTranslator, Base>) {
				luaState.new_usertype<PDFTranslator>("PDFTranslator",
					sol::base_classes, sol::bases<NormalJsonTranslator>(),
					"m_pdfInputDir", PATH_CVT(PDFTranslator, m_pdfInputDir),
					"m_pdfOutputDir", PATH_CVT(PDFTranslator, m_pdfOutputDir),
					"m_bilingualOutput", &PDFTranslator::m_bilingualOutput,
					"m_jsonToPDFPathMap", sol::property([](PDFTranslator& self, sol::this_state s)
						{
							sol::state_view lua(s);
							sol::table infoTable = lua.create_table();
							for (const auto& [key, value] : self.m_jsonToPDFPathMap) {
								infoTable[wide2Ascii(key)] = wide2Ascii(value);
							}
							return infoTable;
						},
						[](PDFTranslator& self, sol::table infoTable)
						{
							self.m_jsonToPDFPathMap.clear();
							for (const auto& kv : infoTable) {
								if (kv.first.is<std::string>() && kv.second.is<std::string>()) {
									self.m_jsonToPDFPathMap.insert({ ascii2Wide(kv.first.as<std::string>()), ascii2Wide(kv.second.as<std::string>()) });
								}
							}
						}),
					"pdfTranslator_run", [](PDFTranslator& self)
					{
						self.PDFTranslator::run();
					}
				);
			}

			luaState.new_usertype<LuaTranslator<Base>>("LuaTranslator",
				sol::base_classes, sol::bases<Base>()
			);
			luaState["luaTranslator"] = this;

			sol::function initFunc = m_luaState->functions["init"];
			try {
				initFunc();
			}
			catch (const sol::error& e) {
				throw std::runtime_error(std::string("初始化 LuaTranslator 失败：") + e.what());
			}

			if (m_needReboot) {
				throw std::runtime_error("LuaTranslator 需要重启程序");
			}
		}

		virtual ~LuaTranslator() override
		{
			if (auto unloadFunc = m_luaState->functions["unload"]; unloadFunc.valid()) {
				unloadFunc();
			}
			this->m_logger->info("所有任务已完成！LuaTranslator {} 结束。", m_translatorName);
		}
	};
}
