module;

#include <spdlog/spdlog.h>

export module PDFTranslator;

import <nlohmann/json.hpp>;
import <toml++/toml.hpp>;
import Tool;
import NormalJsonTranslator;

namespace fs = std::filesystem;
using json = nlohmann::json;

export {

    class PDFTranslator : public NormalJsonTranslator {

    private:

        fs::path m_pdfConverterPath;
        fs::path m_pdfInputDir;
        fs::path m_pdfOutputDir;

        // PDF 处理相关的配置
        bool m_bilingualOutput;

        // 存储json文件相对路径到其所属PDF完整路径的映射
        std::map<fs::path, fs::path> m_jsonToPDFPathMap;

    public:

        virtual void run() override;

        PDFTranslator(const fs::path& projectDir, std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger);

        virtual ~PDFTranslator()
        {
            m_logger->info("所有任务已完成！PDFTranslator结束。");
        }
    };
}



module :private;

PDFTranslator::PDFTranslator(const fs::path& projectDir, std::shared_ptr<IController> controller, std::shared_ptr<spdlog::logger> logger) :
    NormalJsonTranslator(projectDir, controller, logger,
        // m_inputDir                                                m_inputCacheDir
        // m_outputDir                                               m_outputCacheDir
        L"cache" / projectDir.filename() / L"pdf_json_input", L"cache" / projectDir.filename() / L"gt_input_cache",
        L"cache" / projectDir.filename() / L"pdf_json_output", L"cache" / projectDir.filename() / L"gt_output_cache")
{
    m_pdfInputDir = m_projectDir / L"gt_input";
    m_pdfOutputDir = m_projectDir / L"gt_output";

    std::ifstream ifs;

    try {
        ifs.open(m_projectDir / L"config.toml");
        auto projectConfig = toml::parse(ifs);
        ifs.close();

        ifs.open(pluginConfigsPath / L"filePlugins/PDF.toml");
        auto pluginConfig = toml::parse(ifs);
        ifs.close();

        m_bilingualOutput = parseToml<bool>(projectConfig, pluginConfig, "plugins.PDF.输出双语翻译文件");
        std::string pdfConverterPath = parseToml<std::string>(projectConfig, pluginConfig, "plugins.PDF.PDFConverterPath");
        m_pdfConverterPath = ascii2Wide(pdfConverterPath);

        if (!fs::exists(m_pdfConverterPath)) {
            throw std::runtime_error("未找到 PDF 转换器路径");
        }
    }
    catch (const toml::parse_error& e) {
        m_logger->critical("项目配置文件解析失败, 错误位置: {}, 错误信息: {}", stream2String(e.source().begin), e.description());
        throw std::runtime_error(e.what());
    }
}


void PDFTranslator::run()
{
    m_logger->info("GalTransl++ PDFTranslator 启动...");

    for (const auto& dir : { m_pdfInputDir, m_pdfOutputDir }) {
        if (!fs::exists(dir)) {
            fs::create_directories(dir);
            m_logger->info("已创建目录: {}", wide2Ascii(dir));
        }
    }

    for (const auto& dir : { m_inputDir, m_outputDir }) {
        fs::remove_all(dir);
        fs::create_directories(dir);
    }

    std::vector<fs::path> pdfFiles;
    for (const auto& entry : fs::recursive_directory_iterator(m_pdfInputDir)) {
        if (entry.is_regular_file() && isSameExtension(entry.path(), L".pdf")) {
            pdfFiles.push_back(entry.path());
        }
    }
    if (pdfFiles.empty()) {
        throw std::runtime_error("未找到 PDF 文件");
    }
    
    for (const auto& pdfFile : pdfFiles) {
        if (m_controller->shouldStop()) {
            return;
        }
        fs::path relPDFPath = fs::relative(pdfFile, m_pdfInputDir);
        fs::path relJsonPath = fs::path(relPDFPath).replace_extension(".json");

        m_jsonToPDFPathMap[relJsonPath] = pdfFile;
        fs::path inputJsonFile = m_inputDir / relJsonPath;
        createParent(inputJsonFile);
        std::string cmd = wide2Ascii(m_pdfConverterPath) + " extract \"" + wide2Ascii(pdfFile) + "\" \"" + wide2Ascii(inputJsonFile) + "\"";
#ifdef _WIN32
        std::replace(cmd.begin(), cmd.end(), '/', '\\');
#endif
        m_logger->info("正在执行命令: {}", cmd);
        if (!executeCommand(cmd)) {
            throw std::runtime_error("PDF 转换器提取执行失败");
        }
    }

    m_onFileProcessed = [&](const fs::path& relProcessedFile)
        {
            if (!m_jsonToPDFPathMap.contains(relProcessedFile)) {
                m_logger->warn("未找到与 {} 对应的元数据，跳过", wide2Ascii(relProcessedFile));
                return;
            }
            fs::path origPDFPath = m_jsonToPDFPathMap[relProcessedFile];
            fs::path relPDFPath = fs::relative(origPDFPath, m_pdfInputDir);
            fs::path outputPDFFile = m_pdfOutputDir / relPDFPath;
            createParent(outputPDFFile);
            std::string cmd = wide2Ascii(m_pdfConverterPath) + " apply -o \"" + wide2Ascii(outputPDFFile.parent_path()) + "\" ";
            if (!m_bilingualOutput) {
                cmd += "--no-dual ";
            }
            cmd += "\"" + wide2Ascii(origPDFPath) + "\" \"" + wide2Ascii(m_outputDir / relProcessedFile) + "\"";
#ifdef _WIN32
            std::replace(cmd.begin(), cmd.end(), '/', '\\');
#endif
            m_logger->info("正在执行命令: {}", cmd);
            if (!executeCommand(cmd)) {
                throw std::runtime_error("PDF 转换器注回执行失败");
            }
        };

    NormalJsonTranslator::run();
}
