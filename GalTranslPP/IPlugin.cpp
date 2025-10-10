module;

#include <spdlog/spdlog.h>
#include <toml.hpp>

module IPlugin;

import TextPostFull2Half;
import TextLinebreakFix;
import CodePageChecker;

namespace fs = std::filesystem;

IPlugin::IPlugin(const fs::path& projectDir, std::shared_ptr<spdlog::logger> logger) :
	m_projectDir(projectDir),
	m_logger(logger)
{

}

IPlugin::~IPlugin()
{

}

bool IPlugin::needReboot()
{
	return false;
}


std::vector<std::shared_ptr<IPlugin>> registerPlugins(const std::vector<std::string>& pluginNames, const fs::path& projectDir, std::shared_ptr<spdlog::logger> logger,
	const toml::value& projectConfig) {
	std::vector<std::shared_ptr<IPlugin>> plugins;

	for (const auto& pluginName : pluginNames) {
		if (pluginName == "TextPostFull2Half") {
			plugins.push_back(std::make_shared<TextPostFull2Half>(projectDir, projectConfig, logger));
		}
		else if (pluginName == "TextLinebreakFix") {
			plugins.push_back(std::make_shared<TextLinebreakFix>(projectDir, projectConfig, logger));
		}
	}

	return plugins;
}