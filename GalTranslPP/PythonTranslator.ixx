module;

#define _RANGES_
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <spdlog/spdlog.h>

export module PythonTranslator;

import Tool;
import NormalJsonTranslator;
import EpubTranslator;
import PDFTranslator;
import PythonManager;

namespace fs = std::filesystem;
namespace py = pybind11;

export {

	template<typename Base>
	class PythonTranslator : public Base {
	private:
		std::shared_ptr<PythonModule> m_pythonModule;
		std::shared_ptr<py::object> m_pythonRunFunc;
		std::string m_modulePath;
		std::string m_translatorName;
		std::jthread m_runThread;
		bool m_needReboot = false;

	public:
		virtual void run() override
		{
			auto runTaskFunc = [&]()
				{
					try {
						(*m_pythonRunFunc)();
					}
					catch (const std::exception& e) {
						throw std::runtime_error("PythonTranslator 运行时异常: " + std::string(e.what()));
					}
				};
			PythonManager::getInstance().submitTask(std::move(runTaskFunc)).get();
			if (m_runThread.joinable()) {
				m_runThread.join();
			}
		}

		template<typename... Args>
		PythonTranslator(const std::string& modulePath, Args&&... args) :
			Base(std::forward<Args>(args)...), m_modulePath(modulePath)
		{
			m_translatorName = wide2Ascii(fs::path(ascii2Wide(m_modulePath)).filename());
			std::optional<std::shared_ptr<PythonModule>> pythonModuleOpt = PythonManager::getInstance().registerFunction(m_modulePath, "init", this->m_logger, m_needReboot);
			if (!pythonModuleOpt.has_value()) {
				throw std::runtime_error("PythonTranslator 获取 init 函数失败！");
			}
			pythonModuleOpt = PythonManager::getInstance().registerFunction(m_modulePath, "run", this->m_logger, m_needReboot);
			if (!pythonModuleOpt.has_value()) {
				throw std::runtime_error("PythonTranslator 获取 run 函数失败！");
			}
			m_pythonModule = pythonModuleOpt.value();
			m_pythonRunFunc = m_pythonModule->processors_["run"];
			PythonManager::getInstance().registerFunction(m_modulePath, "unload", this->m_logger, m_needReboot);

			this->m_logger->info("PythonTranslator 已加载模块: {}", m_translatorName);

			auto initFuncTaskFunc = [&]()
				{
					py::module_& pythonModule = *(m_pythonModule->module_);
					pythonModule.attr("controller") = this->m_controller;

					if constexpr (std::is_base_of_v<NormalJsonTranslator, Base>) {
						pythonModule.attr("get_trans_engine") = py::cpp_function([this]()
							{
								return this->m_transEngine;
							});
						pythonModule.attr("set_trans_engine") = py::cpp_function([this](TransEngine transEngine)
							{
								this->m_transEngine = transEngine;
							});
						pythonModule.attr("get_project_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_projectDir);
							});
						pythonModule.attr("set_project_dir") = py::cpp_function([this](const std::string& projectDir)
							{
								this->m_projectDir = ascii2Wide(projectDir);
							});
						pythonModule.attr("get_input_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_inputDir);
							});
						pythonModule.attr("set_input_dir") = py::cpp_function([this](const std::string& inputDir)
							{
								this->m_inputDir = ascii2Wide(inputDir);
							});
						pythonModule.attr("get_input_cache_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_inputCacheDir);
							});
						pythonModule.attr("set_input_cache_dir") = py::cpp_function([this](const std::string& inputCacheDir)
							{
								this->m_inputCacheDir = ascii2Wide(inputCacheDir);
							});
						pythonModule.attr("get_output_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_outputDir);
							});
						pythonModule.attr("set_output_dir") = py::cpp_function([this](const std::string& outputDir)
							{
								this->m_outputDir = ascii2Wide(outputDir);
							});
						pythonModule.attr("get_output_cache_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_outputCacheDir);
							});
						pythonModule.attr("set_output_cache_dir") = py::cpp_function([this](const std::string& outputCacheDir)
							{
								this->m_outputCacheDir = ascii2Wide(outputCacheDir);
							});
						pythonModule.attr("get_cache_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_cacheDir);
							});
						pythonModule.attr("set_cache_dir") = py::cpp_function([this](const std::string& cacheDir)
							{
								this->m_cacheDir = ascii2Wide(cacheDir);
							});
						pythonModule.attr("get_systethis->m_prompt") = py::cpp_function([this]()
							{
								return this->m_systemPrompt;
							});
						pythonModule.attr("set_systethis->m_prompt") = py::cpp_function([this](const std::string& systemPrompt)
							{
								this->m_systemPrompt = systemPrompt;
							});
						pythonModule.attr("get_user_prompt") = py::cpp_function([this]()
							{
								return this->m_userPrompt;
							});
						pythonModule.attr("set_user_prompt") = py::cpp_function([this](const std::string& userPrompt)
							{
								this->m_userPrompt = userPrompt;
							});
						pythonModule.attr("get_target_lang") = py::cpp_function([this]()
							{
								return this->m_targetLang;
							});
						pythonModule.attr("set_target_lang") = py::cpp_function([this](const std::string& targetLang)
							{
								this->m_targetLang = targetLang;
							});
						pythonModule.attr("get_total_sentences") = py::cpp_function([this]()
							{
								return this->m_totalSentences;
							});
						pythonModule.attr("set_total_sentences") = py::cpp_function([this](int totalSentences)
							{
								this->m_totalSentences = totalSentences;
							});
						pythonModule.attr("get_completed_sentences") = py::cpp_function([this]()
							{
								return this->m_completedSentences.load();
							});
						pythonModule.attr("set_completed_sentences") = py::cpp_function([this](int completedSentences)
							{
								this->m_completedSentences = completedSentences;
							});
						pythonModule.attr("get_threads_num") = py::cpp_function([this]()
							{
								return this->m_threadsNum;
							});
						pythonModule.attr("set_threads_num") = py::cpp_function([this](int threadsNum)
							{
								this->m_threadsNum = threadsNum;
							});
						pythonModule.attr("get_batch_size") = py::cpp_function([this]()
							{
								return this->m_batchSize;
							});
						pythonModule.attr("set_batch_size") = py::cpp_function([this](int batchSize)
							{
								this->m_batchSize = batchSize;
							});
						pythonModule.attr("get_context_history_size") = py::cpp_function([this]()
							{
								return this->m_contextHistorySize;
							});
						pythonModule.attr("set_context_history_size") = py::cpp_function([this](int contextHistorySize)
							{
								this->m_contextHistorySize = contextHistorySize;
							});
						pythonModule.attr("get_max_retries") = py::cpp_function([this]()
							{
								return this->m_maxRetries;
							});
						pythonModule.attr("set_max_retries") = py::cpp_function([this](int maxRetries)
							{
								this->m_maxRetries = maxRetries;
							});
						pythonModule.attr("get_save_cache_interval") = py::cpp_function([this]()
							{
								return this->m_saveCacheInterval;
							});
						pythonModule.attr("set_save_cache_interval") = py::cpp_function([this](int saveCacheInterval)
							{
								this->m_saveCacheInterval = saveCacheInterval;
							});
						pythonModule.attr("get_api_time_out_ms") = py::cpp_function([this]()
							{
								return this->m_apiTimeOutMs;
							});
						pythonModule.attr("set_api_time_out_ms") = py::cpp_function([this](int apiTimeOutMs)
							{
								this->m_apiTimeOutMs = apiTimeOutMs;
							});
						pythonModule.attr("get_check_quota") = py::cpp_function([this]()
							{
								return this->m_checkQuota;
							});
						pythonModule.attr("set_check_quota") = py::cpp_function([this](bool checkQuota)
							{
								this->m_checkQuota = checkQuota;
							});
						pythonModule.attr("get_smart_retry") = py::cpp_function([this]()
							{
								return this->m_smartRetry;
							});
						pythonModule.attr("set_smart_retry") = py::cpp_function([this](bool smartRetry)
							{
								this->m_smartRetry = smartRetry;
							});
						pythonModule.attr("get_use_pre_dict_in_name") = py::cpp_function([this]()
							{
								return this->m_usePreDictInName;
							});
						pythonModule.attr("set_use_pre_dict_in_name") = py::cpp_function([this](bool usePreDictInName)
							{
								this->m_usePreDictInName = usePreDictInName;
							});
						pythonModule.attr("get_use_post_dict_in_name") = py::cpp_function([this]()
							{
								return this->m_usePostDictInName;
							});
						pythonModule.attr("set_use_post_dict_in_name") = py::cpp_function([this](bool usePostDictInName)
							{
								this->m_usePostDictInName = usePostDictInName;
							});
						pythonModule.attr("get_use_pre_dict_in_msg") = py::cpp_function([this]()
							{
								return this->m_usePreDictInMsg;
							});
						pythonModule.attr("set_use_pre_dict_in_msg") = py::cpp_function([this](bool usePreDictInMsg)
							{
								this->m_usePreDictInMsg = usePreDictInMsg;
							});
						pythonModule.attr("get_use_post_dict_in_msg") = py::cpp_function([this]()
							{
								return this->m_usePostDictInMsg;
							});
						pythonModule.attr("set_use_post_dict_in_msg") = py::cpp_function([this](bool usePostDictInMsg)
							{
								this->m_usePostDictInMsg = usePostDictInMsg;
							});
						pythonModule.attr("get_use_gpt_dict_to_replace_name") = py::cpp_function([this]()
							{
								return this->m_useGptDictToReplaceName;
							});
						pythonModule.attr("set_use_gpt_dict_to_replace_name") = py::cpp_function([this](bool useGptDictToReplaceName)
							{
								this->m_useGptDictToReplaceName = useGptDictToReplaceName;
							});
						pythonModule.attr("get_output_with_src") = py::cpp_function([this]()
							{
								return this->m_outputWithSrc;
							});
						pythonModule.attr("set_output_with_src") = py::cpp_function([this](bool outputWithSrc)
							{
								this->m_outputWithSrc = outputWithSrc;
							});
						pythonModule.attr("get_api_strategy") = py::cpp_function([this]()
							{
								return this->m_apiStrategy;
							});
						pythonModule.attr("set_api_strategy") = py::cpp_function([this](const std::string& apiStrategy)
							{
								this->m_apiStrategy = apiStrategy;
							});
						pythonModule.attr("get_sort_method") = py::cpp_function([this]()
							{
								return this->m_sortMethod;
							});
						pythonModule.attr("set_sort_method") = py::cpp_function([this](const std::string& sortMethod)
							{
								this->m_sortMethod = sortMethod;
							});
						pythonModule.attr("get_split_file") = py::cpp_function([this]()
							{
								return this->m_splitFile;
							});
						pythonModule.attr("set_split_file") = py::cpp_function([this](const std::string& splitFile)
							{
								this->m_splitFile = splitFile;
							});
						pythonModule.attr("get_split_file_num") = py::cpp_function([this]()
							{
								return this->m_splitFileNum;
							});
						pythonModule.attr("set_split_file_num") = py::cpp_function([this](int splitFileNum)
							{
								this->m_splitFileNum = splitFileNum;
							});
						pythonModule.attr("get_linebreak_symbol") = py::cpp_function([this]()
							{
								return this->m_linebreakSymbol;
							});
						pythonModule.attr("set_linebreak_symbol") = py::cpp_function([this](const std::string& linebreakSymbol)
							{
								this->m_linebreakSymbol = linebreakSymbol;
							});
						pythonModule.attr("get_need_combining") = py::cpp_function([this]()
							{
								return this->m_needsCombining;
							});
						pythonModule.attr("set_need_combining") = py::cpp_function([this](bool needCombining)
							{
								this->m_needsCombining = needCombining;
							});
						pythonModule.attr("get_split_file_parts_to_json") = py::cpp_function([this]()
							{
								std::vector<std::string> keys;
								std::vector<std::string> values;
								for (const auto& [key, value] : this->m_splitFilePartsToJson) {
									keys.push_back(wide2Ascii(key));
									values.push_back(wide2Ascii(value));
								}
								return std::make_tuple(keys, values);
							});
						pythonModule.attr("set_split_file_parts_to_json") = py::cpp_function([this](std::vector<std::string> keys, std::vector<std::string> values)
							{
								this->m_splitFilePartsToJson.clear();
								for (const auto& [key, value] : std::views::zip(keys, values)) {
									this->m_splitFilePartsToJson[ascii2Wide(key)] = ascii2Wide(value);
								}
							});
						pythonModule.attr("get_json_to_split_file_parts") = py::cpp_function([this]()
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
							});
						pythonModule.attr("set_json_to_split_file_parts") = py::cpp_function([this](std::vector<std::string> keys, std::vector<std::vector<std::tuple<std::string, bool>>> values)
							{
								this->m_jsonToSplitFileParts.clear();
								for (const auto& [key, value] : std::views::zip(keys, values)) {
									std::map<fs::path, bool> partMap;
									for (const auto& [part, isComplete] : value) {
										partMap[ascii2Wide(part)] = isComplete;
									}
									this->m_jsonToSplitFileParts[ascii2Wide(key)] = partMap;
								}
							});
						pythonModule.attr("get_need_reboot") = py::cpp_function([this]()
							{
								return this->m_needReboot;
							});
						pythonModule.attr("set_need_reboot") = py::cpp_function([this](bool needReboot)
							{
								this->m_needReboot = needReboot;
							});
						pythonModule.attr("get_on_file_processed") = py::cpp_function([this]()
							{
								std::function<void(std::string)> onFileProcessed = [this](std::string relFilePath)
									{
										this->m_onFileProcessed(ascii2Wide(relFilePath));
									};
								return onFileProcessed;
							});
						pythonModule.attr("set_on_file_processed_str") = py::cpp_function([this](std::string funcName)
							{
								// 这里的相对路径是相对 gt_input 来说的
								this->m_onFileProcessed = [funcName, this](fs::path relFilePath)
									{
										auto task = [relFilePath, funcName, this]()
											{
												m_pythonModule->module_->attr(funcName.c_str())(wide2Ascii(relFilePath));
											};
										PythonManager::getInstance().submitTask(std::move(task)).get();
									};
							});
						pythonModule.attr("pre_process") = py::cpp_function([this](Sentence* se)
							{
								this->preProcess(se);
							});
						pythonModule.attr("post_process") = py::cpp_function([this](Sentence* se)
							{
								this->postProcess(se);
							});
						pythonModule.attr("process_file") = py::cpp_function([this](const std::string& file, int threadId)
							{
								this->processFile(ascii2Wide(file), threadId);
							});
						pythonModule.attr("normaljson_translator_run") = py::cpp_function([this]()
							{
								m_runThread = std::jthread([this]
									{
										this->NormalJsonTranslator::run();
									});
							});
					}


					if constexpr (std::is_base_of_v<EpubTranslator, Base>) {
						pythonModule.attr("get_epub_input_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_epubInputDir);
							});
						pythonModule.attr("set_epub_input_dir") = py::cpp_function([this](const std::string& epubInputDir)
							{
								this->m_epubInputDir = ascii2Wide(epubInputDir);
							});
						pythonModule.attr("get_epub_output_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_epubOutputDir);
							});
						pythonModule.attr("set_epub_output_dir") = py::cpp_function([this](const std::string& epubOutputDir)
							{
								this->m_epubOutputDir = ascii2Wide(epubOutputDir);
							});
						pythonModule.attr("get_temp_unpack_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_tempUnpackDir);
							});
						pythonModule.attr("set_temp_unpack_dir") = py::cpp_function([this](const std::string& tempUnpackDir)
							{
								this->m_tempUnpackDir = ascii2Wide(tempUnpackDir);
							});
						pythonModule.attr("get_temp_rebuild_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_tempRebuildDir);
							});
						pythonModule.attr("set_temp_rebuild_dir") = py::cpp_function([this](const std::string& tempRebuildDir)
							{
								this->m_tempRebuildDir = ascii2Wide(tempRebuildDir);
							});
						pythonModule.attr("get_bilingual_output") = py::cpp_function([this]()
							{
								return this->m_bilingualOutput;
							});
						pythonModule.attr("set_bilingual_output") = py::cpp_function([this](bool bilingualOutput)
							{
								this->m_bilingualOutput = bilingualOutput;
							});
						pythonModule.attr("get_original_text_color") = py::cpp_function([this]()
							{
								return this->m_originalTextColor;
							});
						pythonModule.attr("set_original_text_color") = py::cpp_function([this](const std::string& originalTextColor)
							{
								this->m_originalTextColor = originalTextColor;
							});
						pythonModule.attr("get_original_text_scale") = py::cpp_function([this]()
							{
								return this->m_originalTextScale;
							});
						pythonModule.attr("set_original_text_scale") = py::cpp_function([this](const std::string& originalTextScale)
							{
								this->m_originalTextScale = originalTextScale;
							});
						pythonModule.attr("get_json_to_info_map") = py::cpp_function([this]()
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
							});
						pythonModule.attr("set_json_to_info_map") = py::cpp_function([this](std::vector<std::string> keys, std::vector<std::string> htmlPaths, std::vector<std::string> epubPaths, std::vector<std::string> normalPostPaths, std::vector<std::vector<std::tuple<size_t, size_t>>> metadata)
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
							});
						pythonModule.attr("get_epub_to_jsons_map") = py::cpp_function([this]()
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
							});
						pythonModule.attr("set_epub_to_jsons_map") = py::cpp_function([this](std::vector<std::string> keys, std::vector<std::vector<std::tuple<std::string, bool>>> values)
							{
								this->m_epubToJsonsMap.clear();
								for (const auto& [key, value] : std::views::zip(keys, values)) {
									std::map<fs::path, bool> jsonMap;
									for (const auto& [json, isComplete] : value) {
										jsonMap[ascii2Wide(json)] = isComplete;
									}
									this->m_epubToJsonsMap[ascii2Wide(key)] = jsonMap;
								}
							});
						pythonModule.attr("epub_translator_run") = py::cpp_function([this]()
							{
								m_runThread = std::jthread([this]
									{
										this->EpubTranslator::run();
									});
							});
					}


					if constexpr (std::is_base_of_v<PDFTranslator, Base>) {
						pythonModule.attr("get_pdf_input_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_pdfInputDir);
							});
						pythonModule.attr("set_pdf_input_dir") = py::cpp_function([this](const std::string& pdfInputDir)
							{
								this->m_pdfInputDir = ascii2Wide(pdfInputDir);
							});
						pythonModule.attr("get_pdf_output_dir") = py::cpp_function([this]()
							{
								return wide2Ascii(this->m_pdfOutputDir);
							});
						pythonModule.attr("set_pdf_output_dir") = py::cpp_function([this](const std::string& pdfOutputDir)
							{
								this->m_pdfOutputDir = ascii2Wide(pdfOutputDir);
							});
						pythonModule.attr("get_bilingual_output") = py::cpp_function([this]()
							{
								return this->m_bilingualOutput;
							});
						pythonModule.attr("set_bilingual_output") = py::cpp_function([this](bool bilingualOutput)
							{
								this->m_bilingualOutput = bilingualOutput;
							});
						pythonModule.attr("get_json_to_pdf_path_map") = py::cpp_function([this]()
							{
								std::vector<std::string> keys;
								std::vector<std::string> values;
								for (const auto& [key, value] : this->m_jsonToPDFPathMap) {
									keys.push_back(wide2Ascii(key));
									values.push_back(wide2Ascii(value));
								}
								return std::make_tuple(keys, values);
							});
						pythonModule.attr("set_json_to_pdf_path_map") = py::cpp_function([this](std::vector<std::string> keys, std::vector<std::string> values)
							{
								this->m_jsonToPDFPathMap.clear();
								for (const auto& [key, value] : std::views::zip(keys, values)) {
									this->m_jsonToPDFPathMap[ascii2Wide(key)] = ascii2Wide(value);
								}
							});
						pythonModule.attr("pdf_translator_run") = py::cpp_function([this]()
							{
								m_runThread = std::jthread([this]
									{
										this->PDFTranslator::run();
									});
							});
					}

					try {
						pythonModule.attr("init")();
					}
					catch (const pybind11::error_already_set& e) {
						throw std::runtime_error("初始化 PythonTranslator 时发生错误: " + std::string(e.what()));
					}
				};
			PythonManager::getInstance().submitTask(std::move(initFuncTaskFunc)).get();
			if (m_runThread.joinable()) {
				m_runThread.join();
			}
			if (m_needReboot) {
				throw std::runtime_error("PythonTranslator 需要重启程序");
			}
		}

		virtual ~PythonTranslator() override
		{
			auto unloadFuncTaskFunc = [this]()
				{
					if (auto unloadFuncPtr = m_pythonModule->processors_["unload"]; unloadFuncPtr.operator bool()) {
						(*unloadFuncPtr)();
					}
					m_pythonModule->module_->attr("logger") = py::none();
					m_pythonModule->module_->attr("controller") = py::none();
				};
			PythonManager::getInstance().submitTask(std::move(unloadFuncTaskFunc)).get();
			if (m_runThread.joinable()) {
				m_runThread.join();
			}
			this->m_logger->info("所有任务已完成！PythonTranslator {} 结束。", m_translatorName);
		}

	};
}
