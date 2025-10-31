module;

#define PYBIND11_HEADERS
#include "GPPMacros.hpp"
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>
#include <toml.hpp>

export module IPlugin;

export import std;
export import GPPDefines;
export import LuaManager;
export import PythonManager;

namespace fs = std::filesystem;

export {
	class IPlugin {

	protected:

		fs::path m_projectDir;
		std::shared_ptr<spdlog::logger> m_logger;

	public:

		virtual void run(Sentence* se) = 0;

		virtual bool needReboot();

		IPlugin(const fs::path& projectDir, std::shared_ptr<spdlog::logger> logger);

		virtual ~IPlugin();
	};

	std::vector<std::shared_ptr<IPlugin>> registerPlugins(const std::vector<std::string>& pluginNames, const fs::path& projectDir,
		PythonManager& pythonManager, LuaManager& luaManager, std::shared_ptr<spdlog::logger> logger,
		const toml::value& projectConfig);
}