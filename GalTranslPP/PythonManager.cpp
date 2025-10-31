module;

#define PYBIND11_HEADERS
#include "GPPMacros.hpp"
#include <spdlog/spdlog.h>
#include <ctpl_stl.h>

module PythonManager;

import NormalJsonTranslator;
import EpubTranslator;
import PDFTranslator;
import NLPTool;

namespace fs = std::filesystem;
namespace py = pybind11;

// 定义一个 C++ 模块，它将被嵌入到 Python 解释器中
// 所有脚本都可以通过 `import gpp_plugin_api` 来使用这些功能
PYBIND11_EMBEDDED_MODULE(gpp_plugin_api, m, py::multiple_interpreters::per_interpreter_gil()) {

    m.doc() = "C++ API for Python-based plugins";

    py::enum_<NameType>(m, "NameType")
        .value("None", NameType::None)
        .value("Single", NameType::Single)
        .value("Multiple", NameType::Multiple)
        .export_values(); // 允许在 Python 中直接使用 gpp_plugin_api.Single 这样的形式

    py::enum_<TransEngine>(m, "TransEngine")
        .value("None", TransEngine::None)
        .value("ForGalJson", TransEngine::ForGalJson)
        .value("ForGalTsv", TransEngine::ForGalTsv)
        .value("ForNovelTsv", TransEngine::ForNovelTsv)
        .value("DeepseekJson", TransEngine::DeepseekJson)
        .value("Sakura", TransEngine::Sakura)
        .value("DumpName", TransEngine::DumpName)
        .value("GenDict", TransEngine::GenDict)
        .value("Rebuild", TransEngine::Rebuild)
        .value("ShowNormal", TransEngine::ShowNormal)
        .export_values();

    // 绑定 Sentence 结构体
    // 如果指针是 nullptr，在 Python 中会是 None。
    py::class_<Sentence>(m, "Sentence")
        .def(py::init<>()) // 允许在 Python 中创建实例: s = gpp_plugin_api.Sentence()
        .def_readwrite("index", &Sentence::index)
        .def_readwrite("name", &Sentence::name)
        .def_readwrite("names", &Sentence::names) // std::vector<string> <=> list[str]
        .def_readwrite("name_preview", &Sentence::name_preview)
        .def_readwrite("names_preview", &Sentence::names_preview)
        .def_readwrite("original_text", &Sentence::original_text)
        .def_readwrite("pre_processed_text", &Sentence::pre_processed_text)
        .def_readwrite("pre_translated_text", &Sentence::pre_translated_text)
        .def_readwrite("problems", &Sentence::problems)
        .def_readwrite("translated_by", &Sentence::translated_by)
        .def_readwrite("translated_preview", &Sentence::translated_preview)
        .def_readwrite("originalLinebreak", &Sentence::originalLinebreak)
        .def_readwrite("other_info", &Sentence::other_info) // std::map<string, string> <=> dict[str, str]
        .def_readwrite("nameType", &Sentence::nameType)
        .def_readwrite("prev", &Sentence::prev) // Sentence* <=> Sentence or None
        .def_readwrite("next", &Sentence::next) // Sentence* <=> Sentence or None
        .def_readwrite("complete", &Sentence::complete)
        .def_readwrite("notAnalyzeProblem", &Sentence::notAnalyzeProblem)
        .def("problems_get_by_index", &Sentence::problems_get_by_index)
        .def("problems_set_by_index", &Sentence::problems_set_by_index);

    py::enum_<spdlog::level::level_enum>(m, "LogLevel")
        .value("trace", spdlog::level::trace)
        .value("debug", spdlog::level::debug)
        .value("info", spdlog::level::info)
        .value("warn", spdlog::level::warn)
        .value("err", spdlog::level::err)
        .value("critical", spdlog::level::critical)
        .export_values();

    // 绑定 spdlog::logger 类型，以便 Python 知道 "logger" 是什么
    // 使用 std::shared_ptr 作为持有者类型，因为 m_logger 就是一个 shared_ptr
    py::class_<spdlog::logger, std::shared_ptr<spdlog::logger>>(m, "spdlogLogger")
        .def("name", &spdlog::logger::name)
        .def("level", &spdlog::logger::level)
        .def("set_level", &spdlog::logger::set_level)
        .def("set_pattern", [](spdlog::logger& logger, const std::string& pattern) { logger.set_pattern(pattern); })
        .def("trace", [](spdlog::logger& logger, const std::string& msg) { logger.trace(msg); })
        .def("debug", [](spdlog::logger& logger, const std::string& msg) { logger.debug(msg); })
        .def("info", [](spdlog::logger& logger, const std::string& msg) { logger.info(msg); })
        .def("warn", [](spdlog::logger& logger, const std::string& msg) { logger.warn(msg); })
        .def("error", [](spdlog::logger& logger, const std::string& msg) { logger.error(msg); })
        .def("critical", [](spdlog::logger& logger, const std::string& msg) { logger.critical(msg); });

    py::module_ utilsSubmodule = m.def_submodule("utils", "A submodule for utility functions");

    utilsSubmodule
        .def("registerPythonThread", []()
            {
                PythonManager::getInstance().registerPythonThread();
            })
        .def("erasePythonThread", []()
            {
                PythonManager::getInstance().erasePythonThread();
            })
        .def("executeCommand", &executeCommand)
        .def("getConsoleWidth", &getConsoleWidth)
        .def("removePunctuation", &removePunctuation)
        .def("removeWhitespace", &removeWhitespace)
        .def("getMostCommonChar", &getMostCommonChar)
        .def("splitIntoTokens", &splitIntoTokens)
        .def("splitIntoGraphemes", &splitIntoGraphemes)
        .def("extractKatakana", &extractKatakana)
        .def("extractKana", &extractKana)
        .def("extractLatin", &extractLatin)
        .def("extractHangul", &extractHangul)
        .def("getTraditionalChineseExtractor", &getTraditionalChineseExtractor);

    py::class_<IController, std::shared_ptr<IController>>(m, "IController")
        .def("makeBar", &IController::makeBar)
        .def("writeLog", &IController::writeLog)
        .def("addThreadNum", &IController::addThreadNum)
        .def("reduceThreadNum", &IController::reduceThreadNum)
        .def("updateBar", &IController::updateBar)
        .def("shouldStop", &IController::shouldStop)
        .def("flush", &IController::flush);

    // ITranslator
    py::class_<ITranslator>(m, "ITranslator")
        // 不要绑定构造函数，因为 Python 不应该创建这个接口的实例
        .def("run", &ITranslator::run);

    py::class_<ctpl::thread_pool>(m, "ThreadPool")
        .def("resize", &ctpl::thread_pool::resize)
        .def("size", &ctpl::thread_pool::size);

    py::class_<NormalJsonTranslator, ITranslator>(m, "NormalJsonTranslator")
        .def_readwrite("m_transEngine", &NormalJsonTranslator::m_transEngine)
        .def_readwrite("m_controller", &NormalJsonTranslator::m_controller)
        .def_readwrite("m_projectDir", &NormalJsonTranslator::m_projectDir)
        .def_readwrite("m_inputDir", &NormalJsonTranslator::m_inputDir)
        .def_readwrite("m_inputCacheDir", &NormalJsonTranslator::m_inputCacheDir)
        .def_readwrite("m_outputDir", &NormalJsonTranslator::m_outputDir)
        .def_readwrite("m_outputCacheDir", &NormalJsonTranslator::m_outputCacheDir)
        .def_readwrite("m_cacheDir", &NormalJsonTranslator::m_cacheDir)
        .def_readwrite("m_systemPrompt", &NormalJsonTranslator::m_systemPrompt)
        .def_readwrite("m_userPrompt", &NormalJsonTranslator::m_userPrompt)
        .def_readwrite("m_targetLang", &NormalJsonTranslator::m_targetLang)
        .def_readwrite("m_totalSentences", &NormalJsonTranslator::m_totalSentences)
        .def_property("m_completedSentences", [](NormalJsonTranslator& self) {return self.m_completedSentences.load(); },
            [](NormalJsonTranslator& self, int val) { self.m_completedSentences = val; })
        .def_readwrite("m_threadsNum", &NormalJsonTranslator::m_threadsNum)
        .def_readwrite("m_batchSize", &NormalJsonTranslator::m_batchSize)
        .def_readwrite("m_contextHistorySize", &NormalJsonTranslator::m_contextHistorySize)
        .def_readwrite("m_maxRetries", &NormalJsonTranslator::m_maxRetries)
        .def_readwrite("m_saveCacheInterval", &NormalJsonTranslator::m_saveCacheInterval)
        .def_readwrite("m_apiTimeOutMs", &NormalJsonTranslator::m_apiTimeOutMs)
        .def_readwrite("m_checkQuota", &NormalJsonTranslator::m_checkQuota)
        .def_readwrite("m_smartRetry", &NormalJsonTranslator::m_smartRetry)
        .def_readwrite("m_usePreDictInName", &NormalJsonTranslator::m_usePreDictInName)
        .def_readwrite("m_usePostDictInName", &NormalJsonTranslator::m_usePostDictInName)
        .def_readwrite("m_usePreDictInMsg", &NormalJsonTranslator::m_usePreDictInMsg)
        .def_readwrite("m_usePostDictInMsg", &NormalJsonTranslator::m_usePostDictInMsg)
        .def_readwrite("m_useGptDictToReplaceName", &NormalJsonTranslator::m_useGptDictToReplaceName)
        .def_readwrite("m_outputWithSrc", &NormalJsonTranslator::m_outputWithSrc)
        .def_readwrite("m_apiStrategy", &NormalJsonTranslator::m_apiStrategy)
        .def_readwrite("m_sortMethod", &NormalJsonTranslator::m_sortMethod)
        .def_readwrite("m_splitFile", &NormalJsonTranslator::m_splitFile)
        .def_readwrite("m_splitFileNum", &NormalJsonTranslator::m_splitFileNum)
        .def_readwrite("m_linebreakSymbol", &NormalJsonTranslator::m_linebreakSymbol)
        .def_readwrite("m_needsCombining", &NormalJsonTranslator::m_needsCombining)
        .def_readwrite("m_splitFilePartsToJson", &NormalJsonTranslator::m_splitFilePartsToJson)
        .def_readwrite("m_jsonToSplitFileParts", &NormalJsonTranslator::m_jsonToSplitFileParts)
        .def_property("m_onFileProcessed", [](NormalJsonTranslator& self) {return self.m_onFileProcessed; },
            [](NormalJsonTranslator& self, std::function<void(fs::path)> func)
            {
                std::function<void(fs::path)> wrappedFunc = [func](fs::path path)
                    {
                        auto taskFunc = [&]()
                            {
                                func(path);
                            };
                        PythonManager::getInstance().submitTask(std::move(taskFunc)).get();
                    };
                self.m_onFileProcessed = std::move(wrappedFunc);
            })
        .def_property("m_threadPool", [](NormalJsonTranslator& self) -> ctpl::thread_pool& {return self.m_threadPool; }, nullptr, py::return_value_policy::reference_internal)
        .def("preProcess", &NormalJsonTranslator::preProcess)
        .def("postProcess", &NormalJsonTranslator::postProcess)
        .def("processFile", &NormalJsonTranslator::processFile)
        .def("normalJsonTranslator_beforeRun", &NormalJsonTranslator::beforeRun)
        .def("normalJsonTranslator_afterRun", &NormalJsonTranslator::afterRun);

    py::class_<EpubTextNodeInfo>(m, "EpubTextNodeInfo")
        .def(py::init<>())
        .def_readwrite("offset", &EpubTextNodeInfo::offset)
        .def_readwrite("length", &EpubTextNodeInfo::length);

    py::class_<JsonInfo>(m, "JsonInfo")
        .def(py::init<>())
        .def_readwrite("metadata", &JsonInfo::metadata)
        .def_readwrite("htmlPath", &JsonInfo::htmlPath)
        .def_readwrite("epubPath", &JsonInfo::epubPath)
        .def_readwrite("normalPostPath", &JsonInfo::normalPostPath);

    py::class_<EpubTranslator, NormalJsonTranslator>(m, "EpubTranslator")
        .def_readwrite("m_epubInputDir", &EpubTranslator::m_epubInputDir)
        .def_readwrite("m_epubOutputDir", &EpubTranslator::m_epubOutputDir)
        .def_readwrite("m_tempUnpackDir", &EpubTranslator::m_tempUnpackDir)
        .def_readwrite("m_tempRebuildDir", &EpubTranslator::m_tempRebuildDir)
        .def_readwrite("m_bilingualOutput", &EpubTranslator::m_bilingualOutput)
        .def_readwrite("m_originalTextColor", &EpubTranslator::m_originalTextColor)
        .def_readwrite("m_originalTextScale", &EpubTranslator::m_originalTextScale)
        .def_readwrite("m_jsonToInfoMap", &EpubTranslator::m_jsonToInfoMap)
        .def_readwrite("m_epubToJsonsMap", &EpubTranslator::m_epubToJsonsMap)
        .def("epubTranslator_beforeRun", &EpubTranslator::beforeRun);

    py::class_<PDFTranslator, NormalJsonTranslator>(m, "PDFTranslator")
        .def_readwrite("m_pdfInputDir", &PDFTranslator::m_pdfInputDir)
        .def_readwrite("m_pdfOutputDir", &PDFTranslator::m_pdfOutputDir)
        .def_readwrite("m_bilingualOutput", &PDFTranslator::m_bilingualOutput)
        .def_readwrite("m_jsonToPDFPathMap", &PDFTranslator::m_jsonToPDFPathMap)
        .def("pdfTranslator_beforeRun", &PDFTranslator::beforeRun);

}

void PythonManager::checkDependency(const std::vector<std::string>& dependencies, std::shared_ptr<spdlog::logger> logger)
{
    for (const auto& dependency : dependencies) {
        logger->debug("正在检查依赖 {}", dependency);
        auto checkDependencyTaskFunc = [&]()
            {
                try {
                    py::module_::import("importlib.metadata").attr("version")(dependency);
                    logger->debug("依赖 {} 已安装", dependency);
                }
                catch (const py::error_already_set& e) {
                    if (!e.matches(py::module_::import("importlib.metadata").attr("PackageNotFoundError"))) {
                        throw std::runtime_error("检查依赖 " + dependency + " 时出现异常: " + e.what());
                    }
                    logger->error("依赖 {} 未安装，正在尝试安装", dependency);
                    std::string installCommand = "-m pip install " + dependency;
                    logger->info("将在 3s 后开始安装依赖，请勿关闭接下来出现的窗口！");
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                    logger->info("正在执行安装命令: {}", installCommand);
                    if (!executeCommand((fs::absolute(Py_GetPrefix()) / L"python.exe").wstring(), ascii2Wide(installCommand), true)) {
                        throw std::runtime_error("安装依赖 " + dependency + " 的命令失败");
                    }
                    try {
                        py::module_::import("importlib.metadata").attr("version")(dependency);
                        logger->info("依赖 {} 安装成功", dependency);
                    }
                    catch (const py::error_already_set& eReCheck) {
                        throw std::runtime_error("依赖 " + dependency + " 安装验证失败: " + eReCheck.what());
                    }
                }
            };
        PythonManager::getInstance().submitTask(std::move(checkDependencyTaskFunc)).get();
        logger->debug("依赖 {} 检查完毕", dependency);
    }
    logger->debug("所有依赖均已安装");
}

std::shared_ptr<PythonModule> PythonManager::registerNLPFunction(const std::string& moduleName, const std::string& modelName, std::shared_ptr<spdlog::logger> logger, bool& needReboot) {
    
    std::lock_guard<std::recursive_mutex> lock(m_pyModulesMapMutex);
    std::shared_ptr<PythonModule> pythonModule;
    auto it = m_pyModules.find(moduleName);
    if (it != m_pyModules.end() && !it->second.expired()) {
        pythonModule = it->second.lock();
    }
    if (!pythonModule) {
        // 模块不存在，尝试加载
        logger->info("正在加载模块 {}", moduleName);
        auto importTaskFunc = [&]()
            {
                try {
                    std::shared_ptr<py::module_> pyModule = std::shared_ptr<py::module_>(new py::module_{ py::module_::import(moduleName.c_str()) },
                        pythonDeleter<py::module_>);
                    py::module_ importlib = py::module_::import("importlib");
                    importlib.attr("reload")(*pyModule);
                    pythonModule = std::shared_ptr<PythonModule>(new PythonModule{ pyModule, {} });
                    m_pyModules.insert_or_assign(moduleName, pythonModule);
                }
                catch (const py::error_already_set& e) {
                    throw std::runtime_error("加载模块 " + moduleName + " 时出现异常: " + e.what());
                }
            };
        PythonManager::getInstance().submitTask(std::move(importTaskFunc)).get();
        logger->info("模块 {} 已加载", moduleName);
    }

    if (!pythonModule->processors_.contains(modelName)) {
        // 模型类不存在，尝试加载
        logger->info("正在加载模块 {} 的模型 {}", moduleName, modelName);
        auto loadModelClassTaskFunc = [&]()
            {
                try {
                    bool modelInstalled = pythonModule->module_->attr("check_model")(modelName).cast<bool>();
                    if (!modelInstalled) {
                        logger->error("模块 {} 的模型 {} 未安装，正在尝试安装", moduleName, modelName);
                        std::vector<std::string> installArgs = pythonModule->module_->attr("get_download_command")(modelName).cast<std::vector<std::string>>();
                        std::string installCommand;
                        for (const auto& arg : installArgs) {
                            installCommand += arg + " ";
                        }
                        // 这个安装时间长，搞个窗口出来显示进度条吧
                        logger->info("将在 3s 后开始安装模型，请勿关闭接下来出现的窗口！");
                        std::this_thread::sleep_for(std::chrono::seconds(3));
                        logger->info("正在执行安装命令: {}", installCommand);
                        if (!executeCommand((fs::absolute(Py_GetPrefix()) / L"python.exe").wstring(), ascii2Wide(installCommand), true)) {
                            throw std::runtime_error("安装模型 " + modelName + " 的命令失败");
                        }
                        modelInstalled = pythonModule->module_->attr("check_model")(modelName).cast<bool>();
                        if (modelInstalled) {
                            logger->info("模块 {} 的模型 {} 安装成功", moduleName, modelName);
                            pythonModule->processors_.insert(std::make_pair(modelName, std::shared_ptr<py::object>{}));
                            // 不重启会找不到新下载的模型...
                            needReboot = true;
                            return;
                        }
                        else {
                            throw std::runtime_error("模块 " + moduleName + " 的模型 " + modelName + " 安装失败");
                        }
                    }
                    std::shared_ptr<py::object> processorClass = std::shared_ptr<py::object>(new py::object{ pythonModule->module_->attr("NLPProcessor")(modelName) },
                        pythonDeleter<py::object>);
                    pythonModule->processors_.insert(std::make_pair(modelName, processorClass));
                }
                catch (const py::error_already_set& e) {
                    throw std::runtime_error("加载模块 " + moduleName + " 的模型 " + modelName + " 时出现异常: " + e.what());
                }
            };
        PythonManager::getInstance().submitTask(std::move(loadModelClassTaskFunc)).get();
    }
    // 验证一下是不是 空 ptr，比如另一个 Translator 安过但还没重启
    else {
        logger->debug("正在验证模块 {} 的模型 {}", moduleName, modelName);
        if (!pythonModule->processors_[modelName].operator bool()) {
            needReboot = true;
        }
        logger->debug("模块 {} 的模型 {} 已验证", moduleName, modelName);
    }
    logger->debug("模块 {} 的模型 {} 已加载", moduleName, modelName);
    return pythonModule;
}

std::optional<std::shared_ptr<PythonModule>> PythonManager::registerFunction(const std::string& modulePath, const std::string& functionName, std::shared_ptr<spdlog::logger> logger, bool& needReboot) {

    std::lock_guard<std::recursive_mutex> lock(m_pyModulesMapMutex);
    fs::path stdScriptPath = fs::weakly_canonical(ascii2Wide(modulePath));
    if (!fs::exists(stdScriptPath)) {
        logger->error("Script is not found: {}", modulePath);
        return std::nullopt;
    }

    std::string moduleName = wide2Ascii(stdScriptPath.stem());
    std::shared_ptr<PythonModule> pythonModule;
    auto it = m_pyModules.find(moduleName);
    if (it != m_pyModules.end() && !it->second.expired()) {
        pythonModule = it->second.lock();
    }
    if (!pythonModule) {
        std::function<void()> getNLPFuncNeedToDoInThisThread;
        auto loadModuleTaskFunc = [&]()
            {
                try {
                    py::module_ sys = py::module_::import("sys");
                    sys.attr("path").attr("append")(wide2Ascii(stdScriptPath.parent_path()));
                    std::shared_ptr<py::module_> pyModule = std::shared_ptr<py::module_>(new py::module_{ py::module_::import(moduleName.c_str()) },
                        pythonDeleter<py::module_>);
                    py::module_ importlib = py::module_::import("importlib");
                    importlib.attr("reload")(*pyModule);
                    pythonModule = std::shared_ptr<PythonModule>(new PythonModule{ pyModule, {} });
                    m_pyModules.insert_or_assign(moduleName, pythonModule);
                    registerCustomTypes(pythonModule, modulePath, logger, needReboot, getNLPFuncNeedToDoInThisThread);
                }
                catch (const py::error_already_set& e) {
                    throw std::runtime_error("加载模块 " + moduleName + " 时出现异常: " + e.what());
                }
            };
        PythonManager::getInstance().submitTask(std::move(loadModuleTaskFunc)).get();
        if (getNLPFuncNeedToDoInThisThread) {
            getNLPFuncNeedToDoInThisThread();
        }
    }
    if (!pythonModule->processors_.contains(functionName)) {
        bool success = true;
        auto loadFunctionTaskFunc = [&]()
            {
                try {
                    if (!py::hasattr(*pythonModule->module_, functionName.c_str())) {
                        logger->debug("Failed to load function {} from script {}", functionName, modulePath);
                        success = false;
                        return;
                    }
                    py::object func = pythonModule->module_->attr(functionName.c_str());
                    if (!func || !py::isinstance<py::function>(func)) {
                        logger->debug("Failed to load function {} from script {}", functionName, modulePath);
                        success = false;
                        return;
                    }
                    std::shared_ptr<py::object> processorFunc = std::shared_ptr<py::object>(new py::object{ func },
                        pythonDeleter<py::object>);
                    pythonModule->processors_.insert(std::make_pair(functionName, processorFunc));
                }
                catch (const py::error_already_set& e) {
                    throw std::runtime_error("加载模块 " + moduleName + " 的函数 " + functionName + " 时出现异常: " + e.what());
                }
            };
        PythonManager::getInstance().submitTask(std::move(loadFunctionTaskFunc)).get();
        if (!success) {
            return std::nullopt;
        }
    }
    return pythonModule;
}

void PythonManager::registerCustomTypes(std::shared_ptr<PythonModule> pythonModule, const std::string& modulePath, std::shared_ptr<spdlog::logger> logger, bool& needReboot, std::function<void()>& getNLPFunc) {

    auto setupTokenizer = [&](const std::string& mode)
        {
            std::string useTokenizerFlag = mode + "useTokenizer";
            if (py::hasattr(*pythonModule->module_, useTokenizerFlag.c_str()) && pythonModule->module_->attr(useTokenizerFlag.c_str()).cast<bool>()) {
                const std::string tokenizerBackend = pythonModule->module_->attr((mode + "tokenizerBackend").c_str()).cast<std::string>();
                if (tokenizerBackend == "MeCab") {
                    const std::string mecabDictDir = pythonModule->module_->attr((mode + "mecabDictDir").c_str()).cast<std::string>();
                    logger->info("{} 正在检查 MeCab 环境...", modulePath);
                    pythonModule->module_->attr((mode + "tokenizeFunc").c_str()) = getMeCabTokenizeFunc(mecabDictDir, logger);
                    logger->info("{} MeCab 环境检查完毕。", modulePath);
                }
                else if (tokenizerBackend == "spaCy") {
                    const std::string spaCyModelName = pythonModule->module_->attr((mode + "spaCyModelName").c_str()).cast<std::string>();
                    std::function<void()> orgGetNLPFunc = getNLPFunc;
                    getNLPFunc = [=, &needReboot]()
                        {
                            if (orgGetNLPFunc) {
                                orgGetNLPFunc();
                            }
                            std::function<NLPResult(const std::string&)> nlpFunc = getNLPTokenizeFunc({ "spacy" }, "tokenizer_spacy", spaCyModelName, logger, needReboot);
                            auto setNLPTaskFunc = [&]()
                                {
                                    logger->info("{} 正在检查 spaCy 环境...", modulePath);
                                    pythonModule->module_->attr((mode + "tokenizeFunc").c_str()) = std::move(nlpFunc);
                                    logger->info("{} spaCy 环境检查完毕。", modulePath);
                                };
                            PythonManager::getInstance().submitTask(std::move(setNLPTaskFunc)).get();
                        };
                }
                else if (tokenizerBackend == "Stanza") {
                    const std::string stanzaLang = pythonModule->module_->attr((mode + "stanzaLang").c_str()).cast<std::string>();
                    std::function<void()> orgGetNLPFunc = getNLPFunc;
                    getNLPFunc = [=, &needReboot]()
                        {
                            if (orgGetNLPFunc) {
                                orgGetNLPFunc();
                            }
                            std::function<NLPResult(const std::string&)> nlpFunc = getNLPTokenizeFunc({ "stanza" }, "tokenizer_stanza", stanzaLang, logger, needReboot);
                            auto setNLPTaskFunc = [&]()
                                {
                                    logger->info("{} 正在检查 Stanza 环境...", modulePath);
                                    pythonModule->module_->attr((mode + "tokenizeFunc").c_str()) = std::move(nlpFunc);
                                    logger->info("{} Stanza 环境检查完毕。", modulePath);
                                };
                            PythonManager::getInstance().submitTask(std::move(setNLPTaskFunc)).get();
                        };
                }
                else if (tokenizerBackend == "pkuseg") {
                    std::function<void()> orgGetNLPFunc = getNLPFunc;
                    getNLPFunc = [=, &needReboot]()
                        {
                            if (orgGetNLPFunc) {
                                orgGetNLPFunc();
                            }
                            std::function<NLPResult(const std::string&)> nlpFunc = getNLPTokenizeFunc({ "setuptools", "nes-py", "cython", "pkuseg" }, "tokenizer_pkuseg", "default", logger, needReboot);
                            auto setNLPTaskFunc = [&]()
                                {
                                    logger->info("{} 正在检查 pkuseg 环境...", modulePath);
                                    pythonModule->module_->attr((mode + "tokenizeFunc").c_str()) = std::move(nlpFunc);
                                    logger->info("{} pkuseg 环境检查完毕。", modulePath);
                                };
                            PythonManager::getInstance().submitTask(std::move(setNLPTaskFunc)).get();
                        };
                }
                else {
                    throw std::invalid_argument(modulePath + " 无效的 tokenizerBackend: " + tokenizerBackend);
                }
            }
        };
    setupTokenizer("sourceLang_");
    setupTokenizer("targetLang_");
    pythonModule->module_->attr("logger") = (logger);
}

PythonManager& PythonManager::getInstance() {
    static PythonManager instance;
    return instance;
}

PythonManager::PythonManager() {
    // 构造函数只启动线程，不做任何Python操作
    if (Py_IsInitialized()) {
        m_pyThread = std::thread(&PythonManager::run, this);
    }
    else {
        throw std::runtime_error("Python 环境未初始化");
    }
}

PythonManager::~PythonManager() {
    // 类单例的析构都没啥用，直接手动调 stop() 吧
}

void PythonManager::stop() {
    m_taskQueue.stop();
    if (m_pyThread.joinable()) {
        m_pyThread.join();
    }
}

// 守护线程的执行体
void PythonManager::run() {
    py::gil_scoped_acquire acquire;
    try {
        m_pythonThreads.insert(std::this_thread::get_id());
        py::module_::import("gpp_plugin_api");
    }
    catch (const py::error_already_set& e) {
        throw std::runtime_error("import gpp_plugin_api 时出现异常: " + std::string(e.what()));
    }
    while (true) {
        auto taskOpt = m_taskQueue.pop();
        if (!taskOpt) {
            // 队列已停止且为空，退出循环
            break;
        }
        auto task = std::move(*taskOpt);
        try {
            task->taskFunc();
            task->promise.set_value();
        }
        catch (const std::exception&) {
            task->promise.set_exception(std::current_exception());
        }
        catch (...) {
            task->promise.set_exception(std::make_exception_ptr(std::runtime_error("PythonManager::run 出现未知异常")));
        }
    }
}

std::future<void> PythonManager::submitTask(std::function<void()> taskFunc) {
    {
        std::lock_guard<std::mutex> lock(m_threadsSetMutex);
        if (m_pythonThreads.contains(std::this_thread::get_id())) {
            taskFunc();
            return std::async(std::launch::deferred, []() {});
        }
    }
    std::unique_ptr<PythonTask> task = std::make_unique<PythonTask>();
    task->taskFunc = std::move(taskFunc);
    auto future = task->promise.get_future();
    m_taskQueue.push(std::move(task));
    return future;
}
