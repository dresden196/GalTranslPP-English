module;

#include <spdlog/spdlog.h>
#include <sol/sol.hpp>

export module LuaTextPlugin;

import LuaManager;
export import IPlugin;

namespace fs = std::filesystem;

export {

	class LuaTextPlugin : public IPlugin {
	private:
		std::shared_ptr<LuaStateInstance> m_luaState;
		sol::function m_luaRunFunc;
		std::string m_scriptPath;
		bool m_needReboot = false;

	public:
		LuaTextPlugin(const fs::path& projectDir, const std::string& scriptPath, LuaManager& luaManager, std::shared_ptr<spdlog::logger> logger);
		virtual bool needReboot() override { return m_needReboot; }
		virtual void run(Sentence* se) override;
		virtual ~LuaTextPlugin() override;
	};
}


module :private;


LuaTextPlugin::LuaTextPlugin(const fs::path& projectDir, const std::string& scriptPath, LuaManager& luaManager, std::shared_ptr<spdlog::logger> logger)
	: IPlugin(projectDir, logger), m_scriptPath(scriptPath)
{
	m_logger->info("正在初始化 Lua 插件 {}", scriptPath);
	std::optional<std::shared_ptr<LuaStateInstance>> luaStateOpt = luaManager.registerFunction(scriptPath, "init", m_needReboot);
	if (!luaStateOpt.has_value()) {
		throw std::runtime_error(scriptPath + " init函数 初始化失败");
	}
	luaStateOpt = luaManager.registerFunction(scriptPath, "run", m_needReboot);
	if (!luaStateOpt.has_value()) {
		throw std::runtime_error(scriptPath + " run函数 初始化失败");
	}
	m_luaState = luaStateOpt.value();
	m_luaRunFunc = m_luaState->functions["run"];
	luaManager.registerFunction(scriptPath, "unload", m_needReboot);

	try {
		m_luaState->functions["init"](projectDir);
	}
	catch (const sol::error& e) {
		m_logger->error("{} init函数 执行失败", scriptPath);
		throw std::runtime_error(e.what());
	}

	m_logger->info("{} 初始化成功", scriptPath);
}

LuaTextPlugin::~LuaTextPlugin()
{
	sol::function unloadFunc = m_luaState->functions["unload"];
	if (unloadFunc.valid()) {
		try {
			std::lock_guard<std::mutex> lock(m_luaState->executionMutex);
			unloadFunc();
		}
		catch (const sol::error& e) {
			m_logger->error("Error while unloading script: {}", e.what());
		}
	}
}

void LuaTextPlugin::run(Sentence* se) {
	std::lock_guard<std::mutex> lock(m_luaState->executionMutex);
	try {
		m_luaRunFunc(se);
	}
	catch (const sol::error& e) {
		m_logger->error("{} run函数 执行失败", m_scriptPath);
		throw std::runtime_error(e.what());
	}
}
