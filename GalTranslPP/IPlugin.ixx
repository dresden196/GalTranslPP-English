module;

#define PYBIND11_HEADERS
#include "GPPMacros.hpp"
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>
#include <toml.hpp>
#include <proxy/proxy_macros.h>

export module IPlugin;

export import std;
export import GPPDefines;
export import LuaManager;
export import PythonManager;
export import proxy.v4;

namespace fs = std::filesystem;

extern "C++" {
	PRO_DEF_MEM_DISPATCH(MemRun, run);
	PRO_DEF_MEM_DISPATCH(MemNeedReboot, needReboot);
}

export {

	struct PPlugin : pro::facade_builder
		::add_convention<MemRun, void(Sentence*)>
		::add_convention<MemNeedReboot, bool() const>
		::build { };

	std::vector<pro::proxy<PPlugin>> registerPlugins(const std::vector<std::string>& pluginNames, const fs::path& projectDir,
		PythonManager& pythonManager, LuaManager& luaManager, std::shared_ptr<spdlog::logger> logger,
		const toml::value& projectConfig);
}