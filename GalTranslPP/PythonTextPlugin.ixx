module;

#define PYBIND11_HEADERS
#include "GPPMacros.hpp"
#include <toml.hpp>
#include <spdlog/spdlog.h>

export module PythonTextPlugin;

import Tool;
import PythonManager;
export import IPlugin;

namespace fs = std::filesystem;
namespace py = pybind11;

export {

    class PythonTextPlugin : public IPlugin {
    private:
        std::vector<std::function<NLPResult(const std::string&)>> m_tokenizeFuncs;
        std::shared_ptr<PythonModule> m_pyModule;
        std::shared_ptr<py::object> m_pyRunFunc;
        std::string m_modulePath;
        bool m_needReboot = false;

    public:
        PythonTextPlugin(const fs::path& projectDir, const std::string& modulePath, std::shared_ptr<spdlog::logger> logger);
        virtual bool needReboot() override { return m_needReboot; }
        virtual void run(Sentence* se) override;
        virtual ~PythonTextPlugin() override;
    };
}


module :private;

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
                (*m_pyModule->processors_["init"])(projectDir);
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
    auto unloadTaskFunc = [this]()
        {
            try {
                if (auto unloadFunc = m_pyModule->processors_["unload"]; unloadFunc.operator bool() && py::isinstance<py::function>(*unloadFunc)) {
                    (*unloadFunc)();
                }
                //m_pyModule->module_->attr("logger") = py::none();
            }
            catch (const py::error_already_set& e) {
                throw std::runtime_error(m_modulePath + " unload函数 执行失败: " + e.what());
            }
        };
    PythonManager::getInstance().submitTask(std::move(unloadTaskFunc)).get();
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
