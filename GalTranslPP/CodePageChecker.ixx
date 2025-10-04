module;

#include <spdlog/spdlog.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>

export module CodePageChecker;

import <toml.hpp>;
import Tool;
export import IPlugin;

namespace fs = std::filesystem;

export {
    class CodePageChecker : public IPlugin {
    private:
        std::string m_codePage;

    public:
        CodePageChecker(const fs::path& projectDir, std::shared_ptr<spdlog::logger> logger);
        virtual void run(Sentence* se) override;
        virtual ~CodePageChecker() = default;
    };
}

module :private;

CodePageChecker::CodePageChecker(const fs::path& projectDir, std::shared_ptr<spdlog::logger> logger)
    : IPlugin(projectDir, logger)
{
    try {
        const auto projectConfig = toml::parse(projectDir / L"config.toml");
        const auto pluginConfig = toml::parse(pluginConfigsPath / L"textPostPlugins/CodePageChecker.toml");

        m_codePage = parseToml<std::string>(projectConfig, pluginConfig, "codePage");
        m_logger->info("已加载插件 CodePageChecker, 字符集: {}", m_codePage);
    }
    catch (const toml::exception& e) {
        m_logger->critical("字符集检查器 配置文件解析错误");
        throw std::runtime_error(e.what());
    }
}

void CodePageChecker::run(Sentence* se)
{

}
