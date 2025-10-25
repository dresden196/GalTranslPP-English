module;

#define _RANGES_
#include <toml.hpp>
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>
#include <nlohmann/json.hpp>

export module LuaTranslator;

import Tool;
import NormalJsonTranslator;
import LuaManager;

namespace fs = std::filesystem;

export {

	class LuaTranslator : public NormalJsonTranslator {
	private:
		std::shared_ptr<LuaStateInstance> m_luaState;
		sol::function m_luaRunFunc;
		std::string m_scriptPath;
		std::string m_translatorName;
		bool m_needReboot = false;

	public:
		virtual void run() override;

		LuaTranslator(const fs::path& projectDir, const std::string& scriptPath, std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger);

		virtual ~LuaTranslator() override
		{
			m_logger->info("所有任务已完成！LuaTranslator {} 结束。", m_translatorName);
		}
	};
}



module :private;

LuaTranslator::LuaTranslator(const fs::path& projectDir, const std::string& scriptPath, std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger) :
	NormalJsonTranslator(projectDir, controller, logger,
		// m_inputDir                                                m_inputCacheDir
		// m_outputDir                                               m_outputCacheDir
		L"cache" / projectDir.filename() / L"lua_json_input", L"cache" / projectDir.filename() / L"gt_input_cache",
		L"cache" / projectDir.filename() / L"lua_json_output", L"cache" / projectDir.filename() / L"gt_output_cache"), m_scriptPath(scriptPath)
{
	m_translatorName = wide2Ascii(fs::path(ascii2Wide(m_scriptPath)).filename());
	m_inputDir = L"cache" / projectDir.filename() / (ascii2Wide(m_translatorName) + L"_json_input");
	m_outputDir = L"cache" / projectDir.filename() / (ascii2Wide(m_translatorName) + L"_json_output");
	std::optional<std::shared_ptr<LuaStateInstance>> luaStateOpt = m_luaManager.registerFunction(scriptPath, "init", m_needReboot);
	if (!luaStateOpt.has_value()) {
		throw std::runtime_error("获取 init 函数失败。");
	}
	luaStateOpt = m_luaManager.registerFunction(scriptPath, "run", m_needReboot);
	if (!luaStateOpt.has_value()) {
		throw std::runtime_error("获取 run 函数失败。");
	}
	m_luaState = luaStateOpt.value();
	m_luaRunFunc = m_luaState->functions["run"];
	
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
	luaState["controller"] = m_controller;
	luaState["get_trans_engine"] = [this]()
		{
			return m_transEngine;
		};
	luaState["set_trans_engine"] = [this](TransEngine transEngine)
		{
			m_transEngine = transEngine;
		};
	luaState["get_project_dir"] = [this]()
		{
			return wide2Ascii(m_projectDir);
		};
	luaState["set_project_dir"] = [this](const std::string& projectDir)
		{
			m_projectDir = ascii2Wide(projectDir);
		};
	luaState["get_input_dir"] = [this]()
		{
			return wide2Ascii(m_inputDir);
		};
	luaState["set_input_dir"] = [this](const std::string& inputDir)
		{
			m_inputDir = ascii2Wide(inputDir);
		};
	luaState["get_input_cache_dir"] = [this]()
		{
			return wide2Ascii(m_inputCacheDir);
		};
	luaState["set_input_cache_dir"] = [this](const std::string& inputCacheDir)
		{
			m_inputCacheDir = ascii2Wide(inputCacheDir);
		};
	luaState["get_output_dir"] = [this]()
		{
			return wide2Ascii(m_outputDir);
		};
	luaState["set_output_dir"] = [this](const std::string& outputDir)
		{
			m_outputDir = ascii2Wide(outputDir);
		};
	luaState["get_output_cache_dir"] = [this]()
		{
			return wide2Ascii(m_outputCacheDir);
		};
	luaState["set_output_cache_dir"] = [this](const std::string& outputCacheDir)
		{
			m_outputCacheDir = ascii2Wide(outputCacheDir);
		};
	luaState["get_cache_dir"] = [this]()
		{
			return wide2Ascii(m_cacheDir);
		};
	luaState["set_cache_dir"] = [this](const std::string& cacheDir)
		{
			m_cacheDir = ascii2Wide(cacheDir);
		};
	luaState["get_system_prompt"] = [this]()
		{
			return m_systemPrompt;
		};
	luaState["set_system_prompt"] = [this](const std::string& systemPrompt)
		{
			m_systemPrompt = systemPrompt;
		};
	luaState["get_user_prompt"] = [this]()
		{
			return m_userPrompt;
		};
	luaState["set_user_prompt"] = [this](const std::string& userPrompt)
		{
			m_userPrompt = userPrompt;
		};
	luaState["get_target_lang"] = [this]()
		{
			return m_targetLang;
		};
	luaState["set_target_lang"] = [this](const std::string& targetLang)
		{
			m_targetLang = targetLang;
		};
	luaState["get_total_sentences"] = [this]()
		{
			return m_totalSentences;
		};
	luaState["set_total_sentences"] = [this](int totalSentences)
		{
			m_totalSentences = totalSentences;
		};
	luaState["get_completed_sentences"] = [this]()
		{
			return m_completedSentences.load();
		};
	luaState["set_completed_sentences"] = [this](int completedSentences)
		{
			m_completedSentences = completedSentences;
		};
	luaState["get_threads_num"] = [this]()
		{
			return m_threadsNum;
		};
	luaState["set_threads_num"] = [this](int threadsNum)
		{
			m_threadsNum = threadsNum;
		};
	luaState["get_batch_size"] = [this]()
		{
			return m_batchSize;
		};
	luaState["set_batch_size"] = [this](int batchSize)
		{
			m_batchSize = batchSize;
		};
	luaState["get_context_history_size"] = [this]()
		{
			return m_contextHistorySize;
		};
	luaState["set_context_history_size"] = [this](int contextHistorySize)
		{
			m_contextHistorySize = contextHistorySize;
		};
	luaState["get_max_retries"] = [this]()
		{
			return m_maxRetries;
		};
	luaState["set_max_retries"] = [this](int maxRetries)
		{
			m_maxRetries = maxRetries;
		};
	luaState["get_save_cache_interval"] = [this]()
		{
			return m_saveCacheInterval;
		};
	luaState["set_save_cache_interval"] = [this](int saveCacheInterval)
		{
			m_saveCacheInterval = saveCacheInterval;
		};
	luaState["get_api_time_out_ms"] = [this]()
		{
			return m_apiTimeOutMs;
		};
	luaState["set_api_time_out_ms"] = [this](int apiTimeOutMs)
		{
			m_apiTimeOutMs = apiTimeOutMs;
		};
	luaState["get_check_quota"] = [this]()
		{
			return m_checkQuota;
		};
	luaState["set_check_quota"] = [this](bool checkQuota)
		{
			m_checkQuota = checkQuota;
		};
	luaState["get_smart_retry"] = [this]()
		{
			return m_smartRetry;
		};
	luaState["set_smart_retry"] = [this](bool smartRetry)
		{
			m_smartRetry = smartRetry;
		};
	luaState["get_use_pre_dict_in_name"] = [this]()
		{
			return m_usePreDictInName;
		};
	luaState["set_use_pre_dict_in_name"] = [this](bool usePreDictInName)
		{
			m_usePreDictInName = usePreDictInName;
		};
	luaState["get_use_post_dict_in_name"] = [this]()
		{
			return m_usePostDictInName;
		};
	luaState["set_use_post_dict_in_name"] = [this](bool usePostDictInName)
		{
			m_usePostDictInName = usePostDictInName;
		};
	luaState["get_use_pre_dict_in_msg"] = [this]()
		{
			return m_usePreDictInMsg;
		};
	luaState["set_use_pre_dict_in_msg"] = [this](bool usePreDictInMsg)
		{
			m_usePreDictInMsg = usePreDictInMsg;
		};
	luaState["get_use_post_dict_in_msg"] = [this]()
		{
			return m_usePostDictInMsg;
		};
	luaState["set_use_post_dict_in_msg"] = [this](bool usePostDictInMsg)
		{
			m_usePostDictInMsg = usePostDictInMsg;
		};
	luaState["get_use_gpt_dict_to_replace_name"] = [this]()
		{
			return m_useGptDictToReplaceName;
		};
	luaState["set_use_gpt_dict_to_replace_name"] = [this](bool useGptDictToReplaceName)
		{
			m_useGptDictToReplaceName = useGptDictToReplaceName;
		};
	luaState["get_output_with_src"] = [this]()
		{
			return m_outputWithSrc;
		};
	luaState["set_output_with_src"] = [this](bool outputWithSrc)
		{
			m_outputWithSrc = outputWithSrc;
		};
	luaState["get_api_strategy"] = [this]()
		{
			return m_apiStrategy;
		};
	luaState["set_api_strategy"] = [this](const std::string& apiStrategy)
		{
			m_apiStrategy = apiStrategy;
		};
	luaState["get_sort_method"] = [this]()
		{
			return m_sortMethod;
		};
	luaState["set_sort_method"] = [this](const std::string& sortMethod)
		{
			m_sortMethod = sortMethod;
		};
	luaState["get_split_file"] = [this]()
		{
			return m_splitFile;
		};
	luaState["set_split_file"] = [this](const std::string& splitFile)
		{
			m_splitFile = splitFile;
		};
	luaState["get_split_file_num"] = [this]()
		{
			return m_splitFileNum;
		};
	luaState["set_split_file_num"] = [this](int splitFileNum)
		{
			m_splitFileNum = splitFileNum;
		};
	luaState["get_linebreak_symbol"] = [this]()
		{
			return m_linebreakSymbol;
		};
	luaState["set_linebreak_symbol"] = [this](const std::string& linebreakSymbol)
		{
			m_linebreakSymbol = linebreakSymbol;
		};
	luaState["get_need_combining"] = [this]()
		{
			return m_needsCombining;
		};
	luaState["set_need_combining"] = [this](bool needCombining)
		{
			m_needsCombining = needCombining;
		};
	luaState["get_split_file_parts_to_json"] = [this]()
		{
			std::vector<std::string> keys;
			std::vector<std::string> values;
			for (const auto& [key, value] : m_splitFilePartsToJson) {
				keys.push_back(wide2Ascii(key));
				values.push_back(wide2Ascii(value));
			}
			return std::make_tuple(keys, values);
		};
	luaState["set_split_file_parts_to_json"] = [this](std::vector<std::string> keys, std::vector<std::string> values)
		{
			m_splitFilePartsToJson.clear();
			for (const auto& [key, value] : std::views::zip(keys, values)) {
				m_splitFilePartsToJson[ascii2Wide(key)] = ascii2Wide(value);
			}
		};
	luaState["get_json_to_split_file_parts"] = [this]()
		{
			std::vector<std::string> keys;
			std::vector<std::vector<std::tuple<std::string, bool>>> values;
			for (const auto& [key, value] : m_jsonToSplitFileParts) {
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
			m_jsonToSplitFileParts.clear();
			for (const auto& [key, value] : std::views::zip(keys, values)) {
				std::map<fs::path, bool> partMap;
				for (const auto& [part, isComplete] : value) {
					partMap[ascii2Wide(part)] = isComplete;
				}
				m_jsonToSplitFileParts[ascii2Wide(key)] = partMap;
			}
		};
	luaState["get_need_reboot"] = [this]()
		{
			return m_needReboot;
		};
	luaState["set_need_reboot"] = [this](bool needReboot)
		{
			m_needReboot = needReboot;
		};
	luaState["get_on_file_processed"] = [this]()
		{
			std::function<void(std::string)> onFileProcessed = [this](std::string relFilePath)
			{
				m_onFileProcessed(ascii2Wide(relFilePath));
			};
			return onFileProcessed;
		};
	luaState["set_on_file_processed"] = [this](std::function<void(std::string)> onFileProcessed)
		{
			// 这里的相对路径是相对 gt_input 来说的
			m_onFileProcessed = [onFileProcessed](fs::path relFilePath)
				{
					onFileProcessed(wide2Ascii(relFilePath));
				};
		};
	luaState["pre_process"] = [this](Sentence* se)
		{
			preProcess(se);
		};
	luaState["post_process"] = [this](Sentence* se)
		{
			postProcess(se);
		};
	luaState["process_file"] = [this](const std::string& file, int threadId)
		{
			processFile(ascii2Wide(file), threadId);
		};
	luaState["run_normaljson_translator"] = [this]()
		{
			NormalJsonTranslator::run();
		};

	sol::function initFunc = m_luaState->functions["init"];
	try {
		initFunc();
	}
	catch (const sol::error& e) {
		throw std::runtime_error(std::string("初始化 Lua 脚本失败：") + e.what());
	}
}

void LuaTranslator::run()
{
	try {
		m_luaRunFunc();
	}
	catch (const sol::error& e) {
		throw std::runtime_error(std::string("运行 Lua 脚本失败: ") + e.what());
	}
}