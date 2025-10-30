module;

#include "GPPMacros.hpp"
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>
#include <ctpl_stl.h>

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
			luaState.new_usertype<ITranslator>("ITranslator",
				sol::no_constructor,
				"run", &ITranslator::run
			);
			auto threadPoolPushFunc = [](ctpl::thread_pool& self, LuaTranslator<Base>* classPtr, std::vector<fs::path> filePaths)
				{
					std::vector<std::future<void>> futures;
					for (const auto& filePath : filePaths) {
						futures.emplace_back(self.push([classPtr, filePath](const int id)
							{
								classPtr->processFile(filePath, id);
							}));
					}
					for (auto& future : futures) {
						future.get();
					}
				};
			luaState.new_usertype<IController>("IController",
				"makeBar", &IController::makeBar,
				"writeLog", &IController::writeLog,
				"addThreadNum", &IController::addThreadNum,
				"reduceThreadNum", &IController::reduceThreadNum,
				"updateBar", &IController::updateBar,
				"shouldStop", &IController::shouldStop,
				"flush", &IController::flush
			);
			luaState.new_usertype<ctpl::thread_pool>("ThreadPool",
				sol::no_constructor,
				"push", sol::overload(threadPoolPushFunc, [=](ctpl::thread_pool& self, LuaTranslator<Base>* classPtr, std::vector<std::string> filePaths)
					{
						std::vector<fs::path> filePathsVec;
						for (const auto& filePathStr : filePaths) {
							filePathsVec.emplace_back(ascii2Wide(filePathStr));
						}
						threadPoolPushFunc(self, classPtr, filePathsVec);
					}),
				"resize", &ctpl::thread_pool::resize,
				"size", &ctpl::thread_pool::size
			);
			luaState.new_usertype<NormalJsonTranslator>("NormalJsonTranslator",
				sol::base_classes, sol::bases<ITranslator>(),
				"m_transEngine", &NormalJsonTranslator::m_transEngine,
				"m_controller", &NormalJsonTranslator::m_controller,
				"m_projectDir", &NormalJsonTranslator::m_projectDir,
				"m_inputDir", &NormalJsonTranslator::m_inputDir,
				"m_inputCacheDir", &NormalJsonTranslator::m_inputCacheDir,
				"m_outputDir", &NormalJsonTranslator::m_outputDir,
				"m_outputCacheDir", &NormalJsonTranslator::m_outputCacheDir,
				"m_cacheDir", &NormalJsonTranslator::m_cacheDir,
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
				"m_splitFilePartsToJson", NESTED_CVT(NormalJsonTranslator, m_splitFilePartsToJson),
				"m_jsonToSplitFileParts", NESTED_CVT(NormalJsonTranslator, m_jsonToSplitFileParts),
				"m_onFileProcessed", &NormalJsonTranslator::m_onFileProcessed,
				"m_threadPool", &NormalJsonTranslator::m_threadPool,
				"preProcess", &NormalJsonTranslator::preProcess,
				"postProcess", &NormalJsonTranslator::postProcess,
				"processFile", &NormalJsonTranslator::processFile,
				"normalJsonTranslator_beforeRun", &NormalJsonTranslator::beforeRun,
				"normalJsonTranslator_afterRun", &NormalJsonTranslator::afterRun,
				"normalJsonTranslator_run", [](NormalJsonTranslator& self) { self.NormalJsonTranslator::run(); }
			);

			luaState.new_usertype<EpubTextNodeInfo>("EpubTextNodeInfo",
				sol::constructors<EpubTextNodeInfo()>(),
				"offset", &EpubTextNodeInfo::offset,
				"length", &EpubTextNodeInfo::length
			);
			luaState.new_usertype<JsonInfo>("JsonInfo",
				sol::constructors<JsonInfo()>(),
				"metadata", &JsonInfo::metadata,
				"htmlPath", &JsonInfo::htmlPath,
				"epubPath", &JsonInfo::epubPath,
				"normalPostPath", &JsonInfo::normalPostPath
			);
			luaState.new_usertype<EpubTranslator>("EpubTranslator",
				sol::base_classes, sol::bases<ITranslator, NormalJsonTranslator>(),
				"m_epubInputDir", &EpubTranslator::m_epubInputDir,
				"m_epubOutputDir", &EpubTranslator::m_epubOutputDir,
				"m_tempUnpackDir", &EpubTranslator::m_tempUnpackDir,
				"m_tempRebuildDir", &EpubTranslator::m_tempRebuildDir,
				"m_bilingualOutput", &EpubTranslator::m_bilingualOutput,
				"m_originalTextColor", &EpubTranslator::m_originalTextColor,
				"m_originalTextScale", &EpubTranslator::m_originalTextScale,
				"m_jsonToInfoMap", NESTED_CVT(EpubTranslator, m_jsonToInfoMap),
				"m_epubToJsonsMap", NESTED_CVT(EpubTranslator, m_epubToJsonsMap),
				"epubTranslator_beforeRun", &EpubTranslator::beforeRun,
				"epubTranslator_run", [](EpubTranslator& self) { self.EpubTranslator::run(); }
			);

			luaState.new_usertype<PDFTranslator>("PDFTranslator",
				sol::base_classes, sol::bases<ITranslator, NormalJsonTranslator>(),
				"m_pdfInputDir", &PDFTranslator::m_pdfInputDir,
				"m_pdfOutputDir", &PDFTranslator::m_pdfOutputDir,
				"m_bilingualOutput", &PDFTranslator::m_bilingualOutput,
				"m_jsonToPDFPathMap", NESTED_CVT(PDFTranslator, m_jsonToPDFPathMap),
				"pdfTranslator_beforeRun", &PDFTranslator::beforeRun,
				"pdfTranslator_run", [](PDFTranslator& self) { self.PDFTranslator::run(); }
			);

			luaState.new_usertype<LuaTranslator<Base>>("LuaTranslator",
				sol::base_classes, sol::bases<ITranslator,
				std::conditional_t<std::is_base_of_v<NormalJsonTranslator, Base>, NormalJsonTranslator, void>,
				std::conditional_t<std::is_base_of_v<EpubTranslator, Base>, EpubTranslator, void>,
				std::conditional_t<std::is_base_of_v<PDFTranslator, Base>, PDFTranslator, void>
				>()
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
