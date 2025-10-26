module;

#define _RANGES_
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>

export module LuaTranslator;

import Tool;
import NormalJsonTranslator;
import EpubTranslator;
import PDFTranslator;
import LuaManager;

namespace fs = std::filesystem;

export {

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
			luaState.new_enum("TransEngine",
				"None", TransEngine::None,
				"ForGalJson", TransEngine::ForGalJson,
				"ForGalTsv", TransEngine::ForGalTsv,
				"ForNovelTsv", TransEngine::ForNovelTsv,
				"DeepseekJson", TransEngine::DeepseekJson,
				"Sakura", TransEngine::Sakura,
				"DumpName", TransEngine::DumpName,
				"GenDict", TransEngine::GenDict,
				"Rebuild", TransEngine::Rebuild,
				"ShowNormal", TransEngine::ShowNormal
			);
			luaState.new_usertype<IController>("IController",
				"make_bar", &IController::makeBar,
				"write_log", &IController::writeLog,
				"add_thread_num", &IController::addThreadNum,
				"reduce_thread_num", &IController::reduceThreadNum,
				"update_bar", &IController::updateBar,
				"should_stop", &IController::shouldStop,
				"flush", &IController::flush
			);
			luaState["controller"] = this->m_controller;

			if constexpr (std::is_base_of_v<NormalJsonTranslator, Base>) {
				luaState["get_trans_engine"] = [this]()
					{
						return this->m_transEngine;
					};
				luaState["set_trans_engine"] = [this](TransEngine transEngine)
					{
						this->m_transEngine = transEngine;
					};
				luaState["get_project_dir"] = [this]()
					{
						return wide2Ascii(this->m_projectDir);
					};
				luaState["set_project_dir"] = [this](const std::string& projectDir)
					{
						this->m_projectDir = ascii2Wide(projectDir);
					};
				luaState["get_input_dir"] = [this]()
					{
						return wide2Ascii(this->m_inputDir);
					};
				luaState["set_input_dir"] = [this](const std::string& inputDir)
					{
						this->m_inputDir = ascii2Wide(inputDir);
					};
				luaState["get_input_cache_dir"] = [this]()
					{
						return wide2Ascii(this->m_inputCacheDir);
					};
				luaState["set_input_cache_dir"] = [this](const std::string& inputCacheDir)
					{
						this->m_inputCacheDir = ascii2Wide(inputCacheDir);
					};
				luaState["get_output_dir"] = [this]()
					{
						return wide2Ascii(this->m_outputDir);
					};
				luaState["set_output_dir"] = [this](const std::string& outputDir)
					{
						this->m_outputDir = ascii2Wide(outputDir);
					};
				luaState["get_output_cache_dir"] = [this]()
					{
						return wide2Ascii(this->m_outputCacheDir);
					};
				luaState["set_output_cache_dir"] = [this](const std::string& outputCacheDir)
					{
						this->m_outputCacheDir = ascii2Wide(outputCacheDir);
					};
				luaState["get_cache_dir"] = [this]()
					{
						return wide2Ascii(this->m_cacheDir);
					};
				luaState["set_cache_dir"] = [this](const std::string& cacheDir)
					{
						this->m_cacheDir = ascii2Wide(cacheDir);
					};
				luaState["get_systethis->m_prompt"] = [this]()
					{
						return this->m_systemPrompt;
					};
				luaState["set_systethis->m_prompt"] = [this](const std::string& systemPrompt)
					{
						this->m_systemPrompt = systemPrompt;
					};
				luaState["get_user_prompt"] = [this]()
					{
						return this->m_userPrompt;
					};
				luaState["set_user_prompt"] = [this](const std::string& userPrompt)
					{
						this->m_userPrompt = userPrompt;
					};
				luaState["get_target_lang"] = [this]()
					{
						return this->m_targetLang;
					};
				luaState["set_target_lang"] = [this](const std::string& targetLang)
					{
						this->m_targetLang = targetLang;
					};
				luaState["get_total_sentences"] = [this]()
					{
						return this->m_totalSentences;
					};
				luaState["set_total_sentences"] = [this](int totalSentences)
					{
						this->m_totalSentences = totalSentences;
					};
				luaState["get_completed_sentences"] = [this]()
					{
						return this->m_completedSentences.load();
					};
				luaState["set_completed_sentences"] = [this](int completedSentences)
					{
						this->m_completedSentences = completedSentences;
					};
				luaState["get_threads_num"] = [this]()
					{
						return this->m_threadsNum;
					};
				luaState["set_threads_num"] = [this](int threadsNum)
					{
						this->m_threadsNum = threadsNum;
					};
				luaState["get_batch_size"] = [this]()
					{
						return this->m_batchSize;
					};
				luaState["set_batch_size"] = [this](int batchSize)
					{
						this->m_batchSize = batchSize;
					};
				luaState["get_context_history_size"] = [this]()
					{
						return this->m_contextHistorySize;
					};
				luaState["set_context_history_size"] = [this](int contextHistorySize)
					{
						this->m_contextHistorySize = contextHistorySize;
					};
				luaState["get_max_retries"] = [this]()
					{
						return this->m_maxRetries;
					};
				luaState["set_max_retries"] = [this](int maxRetries)
					{
						this->m_maxRetries = maxRetries;
					};
				luaState["get_save_cache_interval"] = [this]()
					{
						return this->m_saveCacheInterval;
					};
				luaState["set_save_cache_interval"] = [this](int saveCacheInterval)
					{
						this->m_saveCacheInterval = saveCacheInterval;
					};
				luaState["get_api_time_out_ms"] = [this]()
					{
						return this->m_apiTimeOutMs;
					};
				luaState["set_api_time_out_ms"] = [this](int apiTimeOutMs)
					{
						this->m_apiTimeOutMs = apiTimeOutMs;
					};
				luaState["get_check_quota"] = [this]()
					{
						return this->m_checkQuota;
					};
				luaState["set_check_quota"] = [this](bool checkQuota)
					{
						this->m_checkQuota = checkQuota;
					};
				luaState["get_smart_retry"] = [this]()
					{
						return this->m_smartRetry;
					};
				luaState["set_smart_retry"] = [this](bool smartRetry)
					{
						this->m_smartRetry = smartRetry;
					};
				luaState["get_use_pre_dict_in_name"] = [this]()
					{
						return this->m_usePreDictInName;
					};
				luaState["set_use_pre_dict_in_name"] = [this](bool usePreDictInName)
					{
						this->m_usePreDictInName = usePreDictInName;
					};
				luaState["get_use_post_dict_in_name"] = [this]()
					{
						return this->m_usePostDictInName;
					};
				luaState["set_use_post_dict_in_name"] = [this](bool usePostDictInName)
					{
						this->m_usePostDictInName = usePostDictInName;
					};
				luaState["get_use_pre_dict_in_msg"] = [this]()
					{
						return this->m_usePreDictInMsg;
					};
				luaState["set_use_pre_dict_in_msg"] = [this](bool usePreDictInMsg)
					{
						this->m_usePreDictInMsg = usePreDictInMsg;
					};
				luaState["get_use_post_dict_in_msg"] = [this]()
					{
						return this->m_usePostDictInMsg;
					};
				luaState["set_use_post_dict_in_msg"] = [this](bool usePostDictInMsg)
					{
						this->m_usePostDictInMsg = usePostDictInMsg;
					};
				luaState["get_use_gpt_dict_to_replace_name"] = [this]()
					{
						return this->m_useGptDictToReplaceName;
					};
				luaState["set_use_gpt_dict_to_replace_name"] = [this](bool useGptDictToReplaceName)
					{
						this->m_useGptDictToReplaceName = useGptDictToReplaceName;
					};
				luaState["get_output_with_src"] = [this]()
					{
						return this->m_outputWithSrc;
					};
				luaState["set_output_with_src"] = [this](bool outputWithSrc)
					{
						this->m_outputWithSrc = outputWithSrc;
					};
				luaState["get_api_strategy"] = [this]()
					{
						return this->m_apiStrategy;
					};
				luaState["set_api_strategy"] = [this](const std::string& apiStrategy)
					{
						this->m_apiStrategy = apiStrategy;
					};
				luaState["get_sort_method"] = [this]()
					{
						return this->m_sortMethod;
					};
				luaState["set_sort_method"] = [this](const std::string& sortMethod)
					{
						this->m_sortMethod = sortMethod;
					};
				luaState["get_split_file"] = [this]()
					{
						return this->m_splitFile;
					};
				luaState["set_split_file"] = [this](const std::string& splitFile)
					{
						this->m_splitFile = splitFile;
					};
				luaState["get_split_file_num"] = [this]()
					{
						return this->m_splitFileNum;
					};
				luaState["set_split_file_num"] = [this](int splitFileNum)
					{
						this->m_splitFileNum = splitFileNum;
					};
				luaState["get_linebreak_symbol"] = [this]()
					{
						return this->m_linebreakSymbol;
					};
				luaState["set_linebreak_symbol"] = [this](const std::string& linebreakSymbol)
					{
						this->m_linebreakSymbol = linebreakSymbol;
					};
				luaState["get_need_combining"] = [this]()
					{
						return this->m_needsCombining;
					};
				luaState["set_need_combining"] = [this](bool needCombining)
					{
						this->m_needsCombining = needCombining;
					};
				luaState["get_split_file_parts_to_json"] = [this]()
					{
						std::vector<std::string> keys;
						std::vector<std::string> values;
						for (const auto& [key, value] : this->m_splitFilePartsToJson) {
							keys.push_back(wide2Ascii(key));
							values.push_back(wide2Ascii(value));
						}
						return std::make_tuple(keys, values);
					};
				luaState["set_split_file_parts_to_json"] = [this](std::vector<std::string> keys, std::vector<std::string> values)
					{
						this->m_splitFilePartsToJson.clear();
						for (const auto& [key, value] : std::views::zip(keys, values)) {
							this->m_splitFilePartsToJson[ascii2Wide(key)] = ascii2Wide(value);
						}
					};
				luaState["get_json_to_split_file_parts"] = [this]()
					{
						std::vector<std::string> keys;
						std::vector<std::vector<std::tuple<std::string, bool>>> values;
						for (const auto& [key, value] : this->m_jsonToSplitFileParts) {
							keys.push_back(wide2Ascii(key));
							std::vector<std::tuple<std::string, bool>> partValues;
							for (const auto& [part, isComplete] : value) {
								partValues.push_back(std::make_tuple(wide2Ascii(part), isComplete));
							}
							values.push_back(partValues);
						}
						return std::make_tuple(keys, values);
					};
				luaState["set_json_to_split_file_parts"] = [this](std::vector<std::string> keys, std::vector<std::vector<std::tuple<std::string, bool>>> values)
					{
						this->m_jsonToSplitFileParts.clear();
						for (const auto& [key, value] : std::views::zip(keys, values)) {
							std::map<fs::path, bool> partMap;
							for (const auto& [part, isComplete] : value) {
								partMap[ascii2Wide(part)] = isComplete;
							}
							this->m_jsonToSplitFileParts[ascii2Wide(key)] = partMap;
						}
					};
				luaState["get_need_reboot"] = [this]()
					{
						return this->m_needReboot;
					};
				luaState["set_need_reboot"] = [this](bool needReboot)
					{
						this->m_needReboot = needReboot;
					};
				luaState["get_on_file_processed"] = [this]()
					{
						std::function<void(std::string)> onFileProcessed = [this](std::string relFilePath)
							{
								this->m_onFileProcessed(ascii2Wide(relFilePath));
							};
						return onFileProcessed;
					};
				luaState["set_on_file_processed"] = [this](std::function<void(std::string)> onFileProcessed)
					{
						// 这里的相对路径是相对 gt_input 来说的
						this->m_onFileProcessed = [onFileProcessed](fs::path relFilePath)
							{
								onFileProcessed(wide2Ascii(relFilePath));
							};
					};
				luaState["pre_process"] = [this](Sentence* se)
					{
						this->preProcess(se);
					};
				luaState["post_process"] = [this](Sentence* se)
					{
						this->postProcess(se);
					};
				luaState["process_file"] = [this](const std::string& file, int threadId)
					{
						this->processFile(ascii2Wide(file), threadId);
					};
				luaState["normaljson_translator_run"] = [this]()
					{
						NormalJsonTranslator::run();
					};
			}


			if constexpr (std::is_base_of_v<EpubTranslator, Base>) {
				luaState["get_epub_input_dir"] = [this]()
					{
						return wide2Ascii(this->m_epubInputDir);
					};
				luaState["set_epub_input_dir"] = [this](const std::string& epubInputDir)
					{
						this->m_epubInputDir = ascii2Wide(epubInputDir);
					};
				luaState["get_epub_output_dir"] = [this]()
					{
						return wide2Ascii(this->m_epubOutputDir);
					};
				luaState["set_epub_output_dir"] = [this](const std::string& epubOutputDir)
					{
						this->m_epubOutputDir = ascii2Wide(epubOutputDir);
					};
				luaState["get_temp_unpack_dir"] = [this]()
					{
						return wide2Ascii(this->m_tempUnpackDir);
					};
				luaState["set_temp_unpack_dir"] = [this](const std::string& tempUnpackDir)
					{
						this->m_tempUnpackDir = ascii2Wide(tempUnpackDir);
					};
				luaState["get_temp_rebuild_dir"] = [this]()
					{
						return wide2Ascii(this->m_tempRebuildDir);
					};
				luaState["set_temp_rebuild_dir"] = [this](const std::string& tempRebuildDir)
					{
						this->m_tempRebuildDir = ascii2Wide(tempRebuildDir);
					};
				luaState["get_bilingual_output"] = [this]()
					{
						return this->m_bilingualOutput;
					};
				luaState["set_bilingual_output"] = [this](bool bilingualOutput)
					{
						this->m_bilingualOutput = bilingualOutput;
					};
				luaState["get_original_text_color"] = [this]()
					{
						return this->m_originalTextColor;
					};
				luaState["set_original_text_color"] = [this](const std::string& originalTextColor)
					{
						this->m_originalTextColor = originalTextColor;
					};
				luaState["get_original_text_scale"] = [this]()
					{
						return this->m_originalTextScale;
					};
				luaState["set_original_text_scale"] = [this](const std::string& originalTextScale)
					{
						this->m_originalTextScale = originalTextScale;
					};
				luaState["get_json_to_info_map"] = [this]()
					{
						std::vector<std::string> keys;
						std::vector<std::string> htmlPaths;
						std::vector<std::string> epubPaths;
						std::vector<std::string> normalPostPaths;
						std::vector<std::vector<std::tuple<size_t, size_t>>> metadata;
						for (const auto& [key, value] : this->m_jsonToInfoMap) {
							keys.push_back(wide2Ascii(key));
							htmlPaths.push_back(wide2Ascii(value.htmlPath));
							epubPaths.push_back(wide2Ascii(value.epubPath));
							normalPostPaths.push_back(wide2Ascii(value.normalPostPath));
							std::vector<std::tuple<size_t, size_t>> metaVec;
							for (const auto& nodeInfo : value.metadata) {
								metaVec.push_back(std::make_tuple(nodeInfo.offset, nodeInfo.length));
							}
							metadata.push_back(metaVec);
						}
						return std::make_tuple(keys, htmlPaths, epubPaths, normalPostPaths, metadata);
					};
				luaState["set_json_to_info_map"] = [this](std::vector<std::string> keys, std::vector<std::string> htmlPaths, std::vector<std::string> epubPaths, std::vector<std::string> normalPostPaths, std::vector<std::vector<std::tuple<size_t, size_t>>> metadata)
					{
						this->m_jsonToInfoMap.clear();
						for (const auto& [key, htmlPath, epubPath, normalPostPath, metaVec] : std::views::zip(keys, htmlPaths, epubPaths, normalPostPaths, metadata)) {
							JsonInfo info;
							info.htmlPath = ascii2Wide(htmlPath);
							info.epubPath = ascii2Wide(epubPath);
							info.normalPostPath = ascii2Wide(normalPostPath);
							for (const auto& [offset, length] : metaVec) {
								info.metadata.emplace_back(EpubTextNodeInfo{ offset, length });
							}
							this->m_jsonToInfoMap[ascii2Wide(key)] = info;
						}
					};
				luaState["get_epub_to_jsons_map"] = [this]()
					{
						std::vector<std::string> keys;
						std::vector<std::vector<std::tuple<std::string, bool>>> values;
						for (const auto& [key, value] : this->m_epubToJsonsMap) {
							keys.push_back(wide2Ascii(key));
							std::vector<std::tuple<std::string, bool>> jsonVec;
							for (const auto& [json, isComplete] : value) {
								jsonVec.push_back(std::make_tuple(wide2Ascii(json), isComplete));
							}
							values.push_back(jsonVec);
						}
						return std::make_tuple(keys, values);
					};
				luaState["set_epub_to_jsons_map"] = [this](std::vector<std::string> keys, std::vector<std::vector<std::tuple<std::string, bool>>> values)
					{
						this->m_epubToJsonsMap.clear();
						for (const auto& [key, value] : std::views::zip(keys, values)) {
							std::map<fs::path, bool> jsonMap;
							for (const auto& [json, isComplete] : value) {
								jsonMap[ascii2Wide(json)] = isComplete;
							}
							this->m_epubToJsonsMap[ascii2Wide(key)] = jsonMap;
						}
					};
				luaState["epub_translator_run"] = [this]()
					{
						EpubTranslator::run();
					};
			}


			if constexpr (std::is_base_of_v<PDFTranslator, Base>) {
				luaState["get_pdf_input_dir"] = [this]()
					{
						return wide2Ascii(this->m_pdfInputDir);
					};
				luaState["set_pdf_input_dir"] = [this](const std::string& pdfInputDir)
					{
						this->m_pdfInputDir = ascii2Wide(pdfInputDir);
					};
				luaState["get_pdf_output_dir"] = [this]()
					{
						return wide2Ascii(this->m_pdfOutputDir);
					};
				luaState["set_pdf_output_dir"] = [this](const std::string& pdfOutputDir)
					{
						this->m_pdfOutputDir = ascii2Wide(pdfOutputDir);
					};
				luaState["get_bilingual_output"] = [this]()
					{
						return this->m_bilingualOutput;
					};
				luaState["set_bilingual_output"] = [this](bool bilingualOutput)
					{
						this->m_bilingualOutput = bilingualOutput;
					};
				luaState["get_json_to_pdf_path_map"] = [this]()
					{
						std::vector<std::string> keys;
						std::vector<std::string> values;
						for (const auto& [key, value] : this->m_jsonToPDFPathMap) {
							keys.push_back(wide2Ascii(key));
							values.push_back(wide2Ascii(value));
						}
						return std::make_tuple(keys, values);
					};
				luaState["set_json_to_pdf_path_map"] = [this](std::vector<std::string> keys, std::vector<std::string> values)
					{
						this->m_jsonToPDFPathMap.clear();
						for (const auto& [key, value] : std::views::zip(keys, values)) {
							this->m_jsonToPDFPathMap[ascii2Wide(key)] = ascii2Wide(value);
						}
					};
				luaState["pdf_translator_run"] = [this]()
					{
						PDFTranslator::run();
					};
			}

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
