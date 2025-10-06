module;

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>
#include <mecab/mecab.h>
#include <toml.hpp>
#include <spdlog/spdlog.h>
#pragma comment(lib, "../lib/python312.lib")

export module PythonManager;

import Tool;

namespace fs = std::filesystem;
namespace py = pybind11;

export {

    template <typename T>
    class SafeQueue {
    public:
        void push(T value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(value));
            m_cond.notify_one();
        }

        std::optional<T> pop() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cond.wait(lock, [this] { return !m_queue.empty() || m_stopped; });
            if (m_stopped && m_queue.empty()) {
                return std::nullopt;
            }
            T value = std::move(m_queue.front());
            m_queue.pop();
            return value;
        }

        void stop() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopped = true;
            m_cond.notify_all();
        }

    private:
        std::queue<T> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        bool m_stopped = false;
    };

    struct PythonTask {
        std::function<void()> taskFunc;
        std::promise<void> promise; // 用于返回结果
    };

    class PythonManager {

    public:
        static PythonManager& getInstance();
        ~PythonManager();

        void stop();

        PythonManager(PythonManager&) = delete;
        PythonManager(PythonManager&&) = delete;

        std::future<void> submitTask(std::function<void()> taskFunc); // 提交任务到队列

        void checkDependency(const std::vector<std::string>& dependencies, std::shared_ptr<spdlog::logger> logger);

    private:
        PythonManager();
        void run(); // 守护线程的执行函数

        std::thread m_pyThread; // 守护线程
        SafeQueue<std::unique_ptr<PythonTask>> m_taskQueue;
    };

    struct NLPModule {
        py::module_ module_;
        std::map<std::string, py::object> processors_;
    };

    class NLPManager {

    public:
        static NLPManager& getInstance();
        ~NLPManager();

        NLPManager(NLPManager&) = delete;
        NLPManager(NLPManager&&) = delete;

        NLPResult processText(const std::string& text, const std::string& moduleName, const std::string& modelName);

        // 每个模块在使用 procecssText 之前，必须先调用该函数检查并加载模块及模型
        bool checkModuleAndModel(const std::vector<std::string>& dependencies, const std::string& moduleName, const std::string& modelName, std::shared_ptr<spdlog::logger> logger);

    private:
        // e.g. { "tokenize_spacy", <pybind11::module, { {"ja_core_news_trf", <pybind11::object>}, {"en_core_web_trf", <pybind11::object> } }> }
        std::map<std::string, NLPModule> m_nlpModules;

        NLPManager();
    };

    std::function<NLPResult(const std::string&)> getMeCabTokenizeFunc(const std::string& mecabDictDir, std::shared_ptr<spdlog::logger> logger);
    std::function<NLPResult(const std::string&)> getNLPTokenizeFunc(const std::vector<std::string>& dependencies, const std::string& moduleName,
        const std::string& modelName, bool& needReboot, std::shared_ptr<spdlog::logger> logger);

    std::tuple<bool, std::string> extractPDF(const fs::path& pdfPath, const fs::path& jsonPath, bool showProgress = false);
    std::tuple<bool, std::string> rejectPDF(const fs::path& orgPDFPath, const fs::path& translatedJsonPath, const fs::path& outputPDFPath,
        bool noMono = false, bool noDual = false, bool showProgress = false);
}


module :private;


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
        catch (const py::error_already_set&) {
            task->promise.set_exception(std::current_exception());
        }
        catch (const std::exception&) {
            task->promise.set_exception(std::current_exception());
        }
    }
}

std::future<void> PythonManager::submitTask(std::function<void()> taskFunc) {
    std::unique_ptr<PythonTask> task = std::make_unique<PythonTask>();
    task->taskFunc = std::move(taskFunc);
    auto future = task->promise.get_future();
    m_taskQueue.push(std::move(task));
    return future;
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
                    catch (const py::error_already_set& e2) {
                        throw std::runtime_error("依赖 " + dependency + " 安装验证失败: " + e2.what());
                    }
                }
            };
        PythonManager::getInstance().submitTask(std::move(checkDependencyTaskFunc)).get();
        logger->info("依赖 {} 检查完毕", dependency);
    }
    logger->info("所有依赖均已安装");
}


NLPManager& NLPManager::getInstance() {
    static NLPManager instance;
    return instance;
}

NLPManager::NLPManager() {

}

NLPManager::~NLPManager() {

}


NLPResult NLPManager::processText(const std::string& text, const std::string& moduleName, const std::string& modelName) {
    
    NLPResult result;
    // 对于直接 get future 的情况，lambda 表达式直接全捕获引用即可
    auto nlpTaskFunc = [&]()
        {
            py::object processor = m_nlpModules[moduleName].processors_[modelName];
            if (processor.is_none()) {
                throw std::runtime_error("未能在模块 " + moduleName + " 中找到模型 " + modelName);
            }
            result = processor.attr("process_text")(text).cast<NLPResult>();
        };

    PythonManager::getInstance().submitTask(std::move(nlpTaskFunc)).get();
    return result;
}

bool NLPManager::checkModuleAndModel(const std::vector<std::string>& dependencies, const std::string& moduleName, const std::string& modelName, std::shared_ptr<spdlog::logger> logger) {

    bool needReboot = false;
    PythonManager::getInstance().checkDependency(dependencies, logger);

    if (!m_nlpModules.contains(moduleName)) {
        // 模块不存在，尝试加载
        logger->info("正在加载模块 {}", moduleName);
        auto importTaskFunc = [&]()
            {
                py::module_ nlpModule = py::module_::import(moduleName.c_str());
                m_nlpModules.insert(std::make_pair(moduleName, NLPModule{ nlpModule, {} }));
            };
        PythonManager::getInstance().submitTask(std::move(importTaskFunc)).get();
        logger->info("模块 {} 已加载", moduleName);
    }

    NLPModule& nlpModuleProcessors = m_nlpModules[moduleName];
    if (!nlpModuleProcessors.processors_.contains(modelName)) {
        // 模型类不存在，尝试加载
        logger->info("正在加载模块 {} 的模型 {}", moduleName, modelName);
        auto loadModelClassTaskFunc = [&]()
            {
                bool modelInstalled = nlpModuleProcessors.module_.attr("check_model")(modelName).cast<bool>();
                if (!modelInstalled) {
                    logger->error("模块 {} 的模型 {} 未安装，正在尝试安装", moduleName, modelName);
                    std::vector<std::string> installArgs = nlpModuleProcessors.module_.attr("get_download_command")(modelName).cast<std::vector<std::string>>();
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
                    modelInstalled = nlpModuleProcessors.module_.attr("check_model")(modelName).cast<bool>();
                    if (modelInstalled) {
                        logger->info("模块 {} 的模型 {} 安装成功", moduleName, modelName);
                        nlpModuleProcessors.processors_.insert(std::make_pair(modelName, py::none()));
                        // 不重启会找不到新下载的模型...
                        needReboot = true;
                        return;
                    }
                    else {
                        throw std::runtime_error("模块 " + moduleName + " 的模型 " + modelName + " 安装失败");
                    }
                }
                py::object processorClass = nlpModuleProcessors.module_.attr("NLPProcessor")(modelName);
                nlpModuleProcessors.processors_.insert(std::make_pair(modelName, processorClass));
            };
        PythonManager::getInstance().submitTask(std::move(loadModelClassTaskFunc)).get();
        logger->info("模块 {} 的模型 {} 已加载", moduleName, modelName);
    }
    // 验证一下是不是 none，比如另一个 Translator 安过但还没重启
    else {
        logger->info("正在验证模块 {} 的模型 {}", moduleName, modelName);
        auto verifyModelTaskFunc = [&]()
            {
                if (nlpModuleProcessors.processors_[modelName].is_none()) {
                    needReboot = true;
                }
            };
        PythonManager::getInstance().submitTask(std::move(verifyModelTaskFunc)).get();
        logger->info("模块 {} 的模型 {} 已验证", moduleName, modelName);
    }

    logger->info("模块 {} 模型 {} 已加载", moduleName, modelName);
    return needReboot;
}


// 这两个返回的函数都是线程安全的
std::function<NLPResult(const std::string&)> getMeCabTokenizeFunc(const std::string& mecabDictDir, std::shared_ptr<spdlog::logger> logger)
{
    std::shared_ptr<MeCab::Model> mecabModel;
    std::shared_ptr<MeCab::Tagger> mecabTagger;
    mecabModel.reset(
        MeCab::Model::create(("-r BaseConfig/mecabDict/mecabrc -d " + ascii2Ascii(mecabDictDir)).c_str())
    );
    if (!mecabModel) {
        throw std::runtime_error("无法初始化 MeCab Model。请确保 BaseConfig/mecabDict/mecabrc 和 " + mecabDictDir + " 存在且无特殊字符\n"
            "错误信息: " + MeCab::getLastError());
    }
    mecabTagger.reset(mecabModel->createTagger());
    if (!mecabTagger) {
        throw std::runtime_error("无法初始化 MeCab Tagger。请确保 BaseConfig/mecabDict/mecabrc 和 " + mecabDictDir + " 存在且无特殊字符\n"
            "错误信息: " + MeCab::getLastError());
    }
    auto resultFunc = [=](const std::string& text) -> NLPResult
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
                logger->trace("分词结果：{} ({})", surface, feature);
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
    const std::string& modelName, bool& needReboot, std::shared_ptr<spdlog::logger> logger)
{
    needReboot = NLPManager::getInstance().checkModuleAndModel(dependencies, moduleName, modelName, logger);
    auto resultFunc = [=](const std::string& text) -> NLPResult
        {
            return NLPManager::getInstance().processText(text, moduleName, modelName);
        };
    return resultFunc;
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