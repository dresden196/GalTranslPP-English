module;

#include <spdlog/spdlog.h>

export module IPlugin;

export import std;
export import GPPDefines;

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
}


module :private;

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
