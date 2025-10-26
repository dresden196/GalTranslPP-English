module;

#define _RANGES_
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <spdlog/spdlog.h>
#include <toml.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <sol/sol.hpp>

module ITranslator;

import Tool;
import LuaManager;
import NormalJsonTranslator;
import EpubTranslator;
import PDFTranslator;
import LuaTranslator;
import PythonTranslator;

namespace fs = std::filesystem;

IController::IController()
{

}

IController::~IController()
{

}

ITranslator::ITranslator()
{

}

ITranslator::~ITranslator()
{

}


template<typename Mutex>
class ControllerSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit ControllerSink(std::shared_ptr<IController> controller)
        : m_controller(controller) {
    }

protected:

    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);
        m_controller->writeLog(fmt::to_string(formatted));
    }

    void flush_() override {
        m_controller->flush();
    }

private:
    std::shared_ptr<IController> m_controller;
};

std::unique_ptr<ITranslator> createTranslator(const fs::path& projectDir, std::shared_ptr<IController> controller)
{
    const fs::path configFilePath = projectDir / L"config.toml";
    if (!fs::exists(configFilePath)) {
        throw std::runtime_error("Config file not found");
    }
    const auto configData = toml::parse(configFilePath);

    const std::string& filePlugin = toml::find_or(configData, "plugins", "filePlugin", "NormalJson");
    const std::string& transEngine = toml::find_or(configData, "plugins", "transEngine", "ForGalJson");
    // 日志配置
    spdlog::level::level_enum logLevel;
    bool saveLog = toml::find_or(configData, "common", "saveLog", true);
    const std::string& logLevelStr = toml::find_or(configData, "common", "logLevel", "info");
    if (logLevelStr == "trace") {
        logLevel = spdlog::level::trace;
    }
    else if (logLevelStr == "debug") {
        logLevel = spdlog::level::debug;
    }
    else if (logLevelStr == "info") {
        logLevel = spdlog::level::info;
    }
    else if (logLevelStr == "warn") {
        logLevel = spdlog::level::warn;
    }
    else if (logLevelStr == "err") {
        logLevel = spdlog::level::err;
    }
    else if (logLevelStr == "critical") {
        logLevel = spdlog::level::critical;
    }
    else {
        throw std::runtime_error("Invalid log level");
    }

    std::shared_ptr<ControllerSink<std::mutex>> controllerSink = std::make_shared<ControllerSink<std::mutex>>(controller);
    std::vector<spdlog::sink_ptr> sinks = { controllerSink };
    if (saveLog) {
        fs::create_directories(projectDir / L"logs");
        for (size_t i = 5; i-- > 0;) {                      // NormalJson_4.log
            fs::path logFilePath = projectDir / L"logs" / (ascii2Wide(transEngine) + L"_" + std::to_wstring(i) + L".log");
            fs::path newLogFilePath = projectDir / L"logs" / (ascii2Wide(transEngine) + L"_" + std::to_wstring(i + 1) + L".log");
            if (!fs::exists(logFilePath)) {
                continue;
            }
            fs::rename(logFilePath, newLogFilePath);
        }
        fs::path logFilePath = projectDir / L"logs" / (ascii2Wide(transEngine) + L"_0.log");
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(wide2Ascii(logFilePath, 0), true));
    }
    std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>(wide2Ascii(projectDir) + "-" + transEngine + "-Logger", sinks.begin(), sinks.end());
    //spdlog::register_logger(logger);
    logger->set_level(logLevel);
    if (logLevel == spdlog::level::trace) {
        logger->flush_on(spdlog::level::trace);
    }
    logger->set_pattern("[%H:%M:%S.%e %^%l%$] %v");
    logger->info("Logger initialized.");
    // 日志配置结束

    std::string filePluginLower = str2Lower(filePlugin);
    if (filePluginLower.starts_with("lua:")) {
        const std::string& baseClassName = toml::find_or(configData, "plugins", "baseClassName", "NormalJson");
        if (baseClassName == "NormalJson") {
            std::unique_ptr<ITranslator> translator = std::make_unique<LuaTranslator<NormalJsonTranslator>>(filePlugin.substr(4), projectDir, controller, logger);
            return translator;
        }
        else if (baseClassName == "Epub") {
            std::unique_ptr<ITranslator> translator = std::make_unique<LuaTranslator<EpubTranslator>>(filePlugin.substr(4), projectDir, controller, logger);
            return translator;
        }
        else if (baseClassName == "PDF") {
            std::unique_ptr<ITranslator> translator = std::make_unique<LuaTranslator<PDFTranslator>>(filePlugin.substr(4), projectDir, controller, logger);
            return translator;
        }
        else {
            throw std::runtime_error("Invalid base class name: " + baseClassName);
        }
    }
    else if (filePluginLower.starts_with("python:")) {
        const std::string& baseClassName = toml::find_or(configData, "plugins", "baseClassName", "NormalJson");
        if (baseClassName == "NormalJson") {
            std::unique_ptr<ITranslator> translator = std::make_unique<PythonTranslator<NormalJsonTranslator>>(filePlugin.substr(7), projectDir, controller, logger);
            return translator;
        }
        else if (baseClassName == "Epub") {
            std::unique_ptr<ITranslator> translator = std::make_unique<PythonTranslator<EpubTranslator>>(filePlugin.substr(7), projectDir, controller, logger);
            return translator;
        }
        else if (baseClassName == "PDF") {
            std::unique_ptr<ITranslator> translator = std::make_unique<PythonTranslator<PDFTranslator>>(filePlugin.substr(7), projectDir, controller, logger);
            return translator;
        }
        else {
            throw std::runtime_error("Invalid base class name: " + baseClassName);
        }
    }
    else if (filePlugin == "NormalJson") {
        std::unique_ptr<ITranslator> translator = std::make_unique<NormalJsonTranslator>(projectDir, controller, logger);
        return translator;
    }
    else if (filePlugin == "Epub") {
        std::unique_ptr<ITranslator> translator = std::make_unique<EpubTranslator>(projectDir, controller, logger);
        return translator;
    }
    else if (filePlugin == "PDF") {
        std::unique_ptr<ITranslator> translator = std::make_unique<PDFTranslator>(projectDir, controller, logger);
        return translator;
    }

    return nullptr;
}