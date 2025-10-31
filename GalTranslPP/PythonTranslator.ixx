module;

#define PYBIND11_HEADERS
#include "GPPMacros.hpp"
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
		}

		template<typename... Args>
		PythonTranslator(const std::string& modulePath, Args&&... args) :
			Base(std::forward<Args>(args)...), m_modulePath(modulePath)
		{
			this->m_pythonTranslator = true;
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

			auto initFuncTaskFunc = [&]()
				{
					try {
						(*(m_pythonModule->module_)).attr("pythonTranslator") = (Base*)this;
						(*(m_pythonModule->processors_["init"]))();
					}
					catch (const pybind11::error_already_set& e) {
						throw std::runtime_error("初始化 PythonTranslator 时发生错误: " + std::string(e.what()));
					}
				};
			PythonManager::getInstance().submitTask(std::move(initFuncTaskFunc)).get();
			if (m_needReboot) {
				throw std::runtime_error("PythonTranslator 需要重启程序");
			}
			this->m_logger->info("PythonTranslator 已加载模块: {}", m_translatorName);
		}

		virtual ~PythonTranslator() override
		{
			auto unloadFuncTaskFunc = [this]()
				{
					try {
						if (auto unloadFuncPtr = m_pythonModule->processors_["unload"]; unloadFuncPtr.operator bool()) {
							(*unloadFuncPtr)();
						}
						//m_pythonModule->module_->attr("logger") = py::none();
						this->m_onFileProcessed = {};
					}
					catch (const py::error_already_set& e) {
						throw std::runtime_error("卸载 PythonTranslator 时发生错误: " + std::string(e.what()));
					}
				};
			PythonManager::getInstance().submitTask(std::move(unloadFuncTaskFunc)).get();
			this->m_logger->info("所有任务已完成！PythonTranslator {} 结束。", m_translatorName);
		}

	};
}
