module;

#define _RANGES_
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <mecab/mecab.h>
#include <toml.hpp>
#include <spdlog/spdlog.h>

module PythonManager;

namespace fs = std::filesystem;
namespace py = pybind11;

// 定义一个 C++ 模块，它将被嵌入到 Python 解释器中
// 所有脚本都可以通过 `import gpp_plugin_api` 来使用这些功能
PYBIND11_EMBEDDED_MODULE(gpp_plugin_api, m) {

    // 绑定 NameType 枚举
    py::enum_<NameType>(m, "NameType")
        .value("None", NameType::None)
        .value("Single", NameType::Single)
        .value("Multiple", NameType::Multiple)
        .export_values(); // 允许在 Python 中直接使用 plugin_api.Single 这样的形式

    // None, ForGalJson, ForGalTsv, ForNovelTsv, DeepseekJson, Sakura, DumpName, GenDict, Rebuild, ShowNormal
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
        .export_values(); // 允许在 Python 中直接使用 plugin_api.ForGalJson 这样的形式

    // 绑定 Sentence 结构体
    // 注意：对于指针成员 prev 和 next，pybind11 会自动处理。
    // 如果指针是 nullptr，在 Python 中会是 None。
    py::class_<Sentence>(m, "Sentence")
        .def(py::init<>()) // 允许在 Python 中创建实例: s = plugin_api.Sentence()
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
        .def("other_info_contains", &Sentence::other_info_contains)
        .def("other_info_get", &Sentence::other_info_get)
        .def("other_info_set", &Sentence::other_info_set)
        .def("other_info_get_all", &Sentence::other_info_get_all)
        .def("other_info_set_all", &Sentence::other_info_set_all)
        .def("other_info_erase", &Sentence::other_info_erase)
        .def("other_info_clear", &Sentence::other_info_clear)
        .def("problems_get_by_index", &Sentence::problems_get_by_index)
        .def("problems_set_by_index", &Sentence::problems_set_by_index);

    // 绑定 spdlog::logger 类型，以便 Python 知道 "logger" 是什么
    // 使用 std::shared_ptr 作为持有者类型，因为 m_logger 就是一个 shared_ptr
    py::class_<spdlog::logger, std::shared_ptr<spdlog::logger>>(m, "Logger")
        .def("trace", [](spdlog::logger& logger, const std::string& msg) { logger.trace(msg); })
        .def("debug", [](spdlog::logger& logger, const std::string& msg) { logger.debug(msg); })
        .def("info", [](spdlog::logger& logger, const std::string& msg) { logger.info(msg); })
        .def("warn", [](spdlog::logger& logger, const std::string& msg) { logger.warn(msg); })
        .def("error", [](spdlog::logger& logger, const std::string& msg) { logger.error(msg); })
        .def("critical", [](spdlog::logger& logger, const std::string& msg) { logger.critical(msg); });

    py::class_<IController, std::shared_ptr<IController>>(m, "IController")
        .def("make_bar", &IController::makeBar)
        .def("write_log", &IController::writeLog)
        .def("add_thread_num", &IController::addThreadNum)
        .def("reduce_thread_num", &IController::reduceThreadNum)
        .def("update_bar", &IController::updateBar)
        .def("should_stop", &IController::shouldStop)
        .def("flush", &IController::flush);
}

PythonTextPlugin::PythonTextPlugin(const fs::path& projectDir, const std::string& modulePath, std::shared_ptr<spdlog::logger> logger)
    : IPlugin(projectDir, logger), m_modulePath(modulePath)
{
    m_logger->info("正在初始化 Python 插件 {}", modulePath);
    std::optional<std::shared_ptr<PythonModule>> pythonModuleOpt = PythonManager::getInstance().registerFunction(m_modulePath, "init", logger, m_needReboot);
    if (!pythonModuleOpt.has_value()) {
        throw std::runtime_error(modulePath + " init函数 初始化失败");
    }
    pythonModuleOpt = PythonManager::getInstance().registerFunction(m_modulePath, "run", logger, m_needReboot);
    if (!pythonModuleOpt.has_value()) {
        throw std::runtime_error(modulePath + " run函数 初始化失败");
    }
    m_pyModule = pythonModuleOpt.value();
    m_pyRunFunc = m_pyModule->processors_["run"];
    PythonManager::getInstance().registerFunction(m_modulePath, "unload", logger, m_needReboot);

    auto initTaskFunc = [&]()
        {
            try {
                (*m_pyModule->processors_["init"])(wide2Ascii(projectDir));
            }
            catch (const py::error_already_set& e) {
                throw std::runtime_error(modulePath + " init函数 执行失败: " + e.what());
            }
        };
    PythonManager::getInstance().submitTask(std::move(initTaskFunc)).get();

    m_logger->info("{} 初始化成功", modulePath);
}

PythonTextPlugin::~PythonTextPlugin()
{
    auto loggerDeleteTaskFunc = [this]()
        {
            try {
                if (auto unloadFunc = m_pyModule->processors_["unload"]; unloadFunc.operator bool() && py::isinstance<py::function>(*unloadFunc)) {
                    (*unloadFunc)();
                }
                m_pyModule->module_->attr("logger") = py::none();
            }
            catch (const py::error_already_set& e) {
                throw std::runtime_error(m_modulePath + " unload函数 执行失败: " + e.what());
            }
        };
    PythonManager::getInstance().submitTask(std::move(loggerDeleteTaskFunc)).get();
}

void PythonTextPlugin::run(Sentence* se) {
    auto runTaskFunc = [&]()
        {
            try {
                (*m_pyRunFunc)(se); // 使用 py::object 的 operator() 来调用 Python 函数
            }
            catch (const py::error_already_set& e) {
                throw std::runtime_error(m_modulePath + " run函数 执行失败: " + e.what());
            }
        };
    PythonManager::getInstance().submitTask(std::move(runTaskFunc)).get();
}

void PythonManager::checkDependency(const std::vector<std::string>& dependencies, std::shared_ptr<spdlog::logger> logger)
{
    for (const auto& dependency : dependencies) {
        logger->info("正在检查依赖 {}", dependency);
        auto checkDependencyTaskFunc = [&]()
            {
                try {
                    py::module_::import("importlib.metadata").attr("version")(dependency);
                    logger->info("依赖 {} 已安装", dependency);
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
        logger->info("依赖 {} 检查完毕", dependency);
    }
    logger->info("所有依赖均已安装");
}

std::shared_ptr<PythonModule> PythonManager::registerNLPFunction(const std::string& moduleName, const std::string& modelName, std::shared_ptr<spdlog::logger> logger, bool& needReboot) {
    
    std::unique_lock<std::mutex> lock(m_mutex);
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
        logger->info("正在验证模块 {} 的模型 {}", moduleName, modelName);
        if (!pythonModule->processors_[modelName].operator bool()) {
            needReboot = true;
        }
        logger->info("模块 {} 的模型 {} 已验证", moduleName, modelName);
    }
    logger->info("模块 {} 的模型 {} 已加载", moduleName, modelName);
    return pythonModule;
}

std::optional<std::shared_ptr<PythonModule>> PythonManager::registerFunction(const std::string& modulePath, const std::string& functionName, std::shared_ptr<spdlog::logger> logger, bool& needReboot) {

    std::unique_lock<std::mutex> lock(m_mutex);
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
                    // 这里在注册的时候可能会获取 NLP 函数导致进入 registerNLPFunction 形成死锁，所以先释放锁
                    lock.unlock();
                    registerCustomTypes(pythonModule, modulePath, logger, needReboot);
                    lock.lock();
                }
                catch (const py::error_already_set& e) {
                    throw std::runtime_error("加载模块 " + moduleName + " 时出现异常: " + e.what());
                }
            };
        PythonManager::getInstance().submitTask(std::move(loadModuleTaskFunc)).get();
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

void PythonManager::registerCustomTypes(std::shared_ptr<PythonModule> pythonModule, const std::string& modulePath, std::shared_ptr<spdlog::logger> logger, bool& needReboot) {

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
                    logger->info("{} 正在检查 spaCy 环境...", modulePath);
                    pythonModule->module_->attr((mode + "tokenizeFunc").c_str()) = getNLPTokenizeFunc({ "spacy" }, "tokenizer_spacy", spaCyModelName, logger, needReboot);
                    logger->info("{} spaCy 环境检查完毕。", modulePath);
                }
                else if (tokenizerBackend == "Stanza") {
                    const std::string stanzaLang = pythonModule->module_->attr((mode + "stanzaLang").c_str()).cast<std::string>();
                    logger->info("{} 正在检查 Stanza 环境...", modulePath);
                    pythonModule->module_->attr((mode + "tokenizeFunc").c_str()) = getNLPTokenizeFunc({ "stanza" }, "tokenizer_stanza", stanzaLang, logger, needReboot);
                    logger->info("{} Stanza 环境检查完毕。", modulePath);
                }
                else if (tokenizerBackend == "pkuseg") {
                    logger->info("{} 正在检查 pkuseg 环境...", modulePath);
                    pythonModule->module_->attr((mode + "tokenizeFunc").c_str()) = getNLPTokenizeFunc({ "setuptools", "nes-py", "cython", "pkuseg" }, "tokenizer_pkuseg", "default", logger, needReboot);
                    logger->info("{} pkuseg 环境检查完毕。", modulePath);
                }
                else {
                    throw std::invalid_argument(modulePath + " 无效的 tokenizerBackend: " + tokenizerBackend);
                }
            }
        };
    setupTokenizer("sourceLang_");
    setupTokenizer("targetLang_");
    pythonModule->module_->attr("logger") = logger;
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
        m_threadId = std::this_thread::get_id();
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
    if (m_threadId == std::this_thread::get_id()) {
        taskFunc();
        return std::async(std::launch::deferred, []() {});
    }
    std::unique_ptr<PythonTask> task = std::make_unique<PythonTask>();
    task->taskFunc = std::move(taskFunc);
    auto future = task->promise.get_future();
    m_taskQueue.push(std::move(task));
    return future;
}


// 这两个返回的函数都是线程安全的
std::function<NLPResult(const std::string&)> getMeCabTokenizeFunc(const std::string& mecabDictDir, std::shared_ptr<spdlog::logger> logger)
{
    std::shared_ptr<MeCab::Model> mecabModel = std::shared_ptr<MeCab::Model>(
        MeCab::Model::create(("-r BaseConfig/mecabDict/mecabrc -d " + ascii2Ascii(mecabDictDir)).c_str())
    );
    if (!mecabModel) {
        throw std::runtime_error("无法初始化 MeCab Model。请确保 BaseConfig/mecabDict/mecabrc 和 " + mecabDictDir + " 存在且无特殊字符\n"
            "错误信息: " + MeCab::getLastError());
    }
    std::shared_ptr<MeCab::Tagger> mecabTagger = std::shared_ptr<MeCab::Tagger>(mecabModel->createTagger());
    if (!mecabTagger) {
        throw std::runtime_error("无法初始化 MeCab Tagger。请确保 BaseConfig/mecabDict/mecabrc 和 " + mecabDictDir + " 存在且无特殊字符\n"
            "错误信息: " + MeCab::getLastError());
    }
    std::function<NLPResult(const std::string&)> resultFunc = [=](const std::string& text) -> NLPResult
        {
            WordPosVec wordPosList;
            EntityVec entityList;
            std::unique_ptr<MeCab::Lattice> lattice(mecabModel->createLattice());
            lattice->set_sentence(text.c_str());
            if (!mecabTagger->parse(lattice.get())) {
                throw std::runtime_error(std::format("分词器解析失败，错误信息: {}", MeCab::getLastError()));
            }
            for (const MeCab::Node* node = lattice->bos_node(); node; node = node->next) {
                if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) continue;

                std::string surface(node->surface, node->length);
                std::string feature = node->feature;
                //logger->trace("分词结果：{} ({})", surface, feature);
                wordPosList.emplace_back(std::vector<std::string>{ surface, feature });
                if (feature.find("固有名詞") != std::string::npos || !extractKatakana(surface).empty()) {
                    entityList.emplace_back(std::vector<std::string>{ surface, feature });
                }
            }
            return NLPResult{ std::move(wordPosList), std::move(entityList) };
        };
    return resultFunc;
}
std::function<NLPResult(const std::string&)> getNLPTokenizeFunc(const std::vector<std::string>& dependencies, const std::string& moduleName,
    const std::string& modelName, std::shared_ptr<spdlog::logger> logger, bool& needReboot)
{
    PythonManager::getInstance().checkDependency(dependencies, logger);
    std::shared_ptr<PythonModule> pythonModule = PythonManager::getInstance().registerNLPFunction(moduleName, modelName, logger, needReboot);
    std::shared_ptr<py::object> processorClass = pythonModule->processors_[modelName];
    std::function<NLPResult(const std::string&)> resultFunc;
    if (processorClass) {
        std::shared_ptr<py::object> processorFunc;
        auto getFuncTaskFunc = [&]()
            {
                processorFunc = std::shared_ptr<py::object>(new py::object{ processorClass->attr("process_text") }, pythonDeleter<py::object>);
            };
        PythonManager::getInstance().submitTask(std::move(getFuncTaskFunc)).get();
        resultFunc = [pythonModule, processorFunc](const std::string& text) -> NLPResult
            {
                NLPResult result;
                auto nlpTaskFunc = [&]()
                    {
                        result = (*processorFunc)(text).cast<NLPResult>();
                    };
                PythonManager::getInstance().submitTask(std::move(nlpTaskFunc)).get();
                return result;
            };
    }
    else {
        resultFunc = [](const std::string& text) -> NLPResult
            {
                return NLPResult{ { {text, "ORIG"} }, EntityVec{} };
            };
    }
    return resultFunc;
}

std::optional<CheckSeCondFunc> getPythonCheckSeCondFunc(const std::string& modulePath, const std::string& functionName, std::shared_ptr<spdlog::logger> logger, bool& needReboot)
{
    std::optional<std::shared_ptr<PythonModule>> pythonModuleOpt = PythonManager::getInstance().registerFunction(modulePath, functionName, logger, needReboot);
    if (!pythonModuleOpt) {
        return std::nullopt;
    }
    std::shared_ptr<PythonModule> pythonModule = *pythonModuleOpt;
    std::shared_ptr<py::object> conditionFunc = pythonModule->processors_[functionName];
    CheckSeCondFunc checkFunc = [pythonModule, conditionFunc, functionName](const Sentence* se) -> bool
        {
            bool result;
            auto checkTaskFunc = [&]()
                {
                    try {
                        result = (*conditionFunc)(se).cast<bool>();
                    }
                    catch (const py::error_already_set& e) {
                        throw std::runtime_error(std::format("执行Python条件函数 {} 时发生错误: {}", functionName, e.what()));
                    }
                };
            PythonManager::getInstance().submitTask(std::move(checkTaskFunc)).get();
            return result;
        };
    return checkFunc;
}

std::tuple<bool, std::string> extractPDF(const fs::path& pdfPath, const fs::path& jsonPath, bool showProgress)
{
    bool success = false;
    std::string message;
    auto extractTaskFunc = [&]()
        {
            std::tie(success, message) = py::module_::import("PDFConverter").attr("api_extract")
                (pdfPath.wstring(), jsonPath.wstring(), showProgress).cast<std::tuple<bool, std::string>>();
        };
    PythonManager::getInstance().submitTask(std::move(extractTaskFunc)).get();
    return std::make_tuple(success, message);
}
std::tuple<bool, std::string> rejectPDF(const fs::path& orgPDFPath, const fs::path& translatedJsonPath, const fs::path& outputPDFPath,
    bool noMono, bool noDual, bool showProgress)
{
    bool success = false;
    std::string message;
    auto rejectTaskFunc = [&]()
        {
            std::tie(success, message) = py::module_::import("PDFConverter").attr("api_apply")
                (orgPDFPath.wstring(), translatedJsonPath.wstring(), outputPDFPath.wstring(), noMono, noDual, showProgress).cast<std::tuple<bool, std::string>>();
        };
    PythonManager::getInstance().submitTask(std::move(rejectTaskFunc)).get();
    return std::make_tuple(success, message);
}
