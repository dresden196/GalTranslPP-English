module;

#define _RANGES_
#include <spdlog/spdlog.h>
#include <toml.hpp>
#include <sol/sol.hpp>

module NJ_ImplTool;

import PythonManager;
import TextPostFull2Half;
import TextLinebreakFix;
import SkipTrans;

namespace fs = std::filesystem;

std::vector<std::shared_ptr<IPlugin>> registerPlugins(const std::vector<std::string>& pluginNames, const fs::path& projectDir,
    LuaManager& luaManager, std::shared_ptr<spdlog::logger> logger,
    const toml::value& projectConfig) {
    std::vector<std::shared_ptr<IPlugin>> plugins;

    for (const auto& pluginName : pluginNames) {
        std::string pluginNameLower = str2Lower(pluginName);
        if (pluginNameLower.starts_with("lua:")) {
            std::string luaScriptPath = pluginName.substr(4);
            plugins.push_back(std::make_shared<LuaTextPlugin>(projectDir, replaceStrInplace(luaScriptPath, "<PROJECT_DIR>", wide2Ascii(projectDir)), luaManager, logger));
        }
        else if (pluginNameLower.starts_with("python:")) {
            std::string pythonModulePath = pluginName.substr(7);
            plugins.push_back(std::make_shared<PythonTextPlugin>(projectDir, replaceStrInplace(pythonModulePath, "<PROJECT_DIR>", wide2Ascii(projectDir)), logger));
        }
        else if (pluginName == "TextPostFull2Half") {
            plugins.push_back(std::make_shared<TextPostFull2Half>(projectDir, projectConfig, logger));
        }
        else if (pluginName == "TextLinebreakFix") {
            plugins.push_back(std::make_shared<TextLinebreakFix>(projectDir, projectConfig, logger));
        }
        else if (pluginName == "SkipTrans") {
            plugins.push_back(std::make_shared<SkipTrans>(projectDir, projectConfig, luaManager, logger));
        }
    }

    return plugins;
}
