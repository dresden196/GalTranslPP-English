module;

#include <spdlog/spdlog.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>
#include <unicode/ustring.h>
#include <toml.hpp>

export module CodePageChecker;

import Tool;
export import IPlugin;

namespace fs = std::filesystem;

export {

    void U_CALLCONV codePageFromUCallback(
        const void* context,
        UConverterFromUnicodeArgs* fromUArgs,
        const UChar* codeUnits,
        int32_t length,
        UChar32 codePoint,
        UConverterCallbackReason reason,
        UErrorCode* pErrorCode);

    struct UConverterDeleter {
        void operator()(UConverter* converter) const {
            ucnv_close(converter);
        }
    };
    using UConverterPtr = std::unique_ptr<UConverter, UConverterDeleter>;

    class CodePageChecker : public IPlugin {
    private:
        std::string m_codePage;
        std::string m_unmappableCharsResult;
        UConverterPtr m_u8Converter;
        UConverterPtr m_codePageConverter;

        const std::string& findUnmappableChars(const std::string& transViewToCheck);

    public:
        CodePageChecker(const fs::path& projectDir, const toml::value& projectConfig, std::shared_ptr<spdlog::logger> logger);
        virtual void run(Sentence* se) override;
        virtual ~CodePageChecker() override = default;
    };
}

module :private;

CodePageChecker::CodePageChecker(const fs::path& projectDir, const toml::value& projectConfig, std::shared_ptr<spdlog::logger> logger)
    : IPlugin(projectDir, logger)
{
    try {
        const auto pluginConfig = toml::parse(pluginConfigsPath / L"textPostPlugins/CodePageChecker.toml");

        m_codePage = parseToml<std::string>(projectConfig, pluginConfig, "plugins.CodePageChecker.codePage");

        UErrorCode status = U_ZERO_ERROR;
        m_u8Converter.reset(ucnv_open("utf-8", &status));
        if (U_FAILURE(status)) {
            throw std::runtime_error("Error: Could not create ICU u8 converters. " + std::string(u_errorName(status)));
        }
        m_codePageConverter.reset(ucnv_open(m_codePage.c_str(), &status));
        if (U_FAILURE(status)) {
            throw std::runtime_error("Error: Could not create ICU " + m_codePage + " converters. " + std::string(u_errorName(status)));
        }
        ucnv_setFromUCallBack(m_codePageConverter.get(), codePageFromUCallback, &m_unmappableCharsResult, nullptr, nullptr, &status);
        if (U_FAILURE(status)) {
            throw std::runtime_error("Error: Could not set ICU callback function. " + std::string(u_errorName(status)));
        }

        m_logger->info("已加载插件 CodePageChecker, 字符集: {}", m_codePage);
    }
    catch (const toml::exception& e) {
        m_logger->critical("字符集检查器 配置文件解析错误");
        throw std::runtime_error(e.what());
    }
}

/**
 * @brief ICU 回调函数，用于记录不能被转换为codePage的字符。
 *
 * @param context 指向 std::string 的指针，用于记录不能被转换的字符。
 * @param fromUArgs 转换参数。
 * @param codeUnits 输入的码点序列。
 * @param length 码点序列长度。
 * @param codePoint 输入的码点。
 * @param reason 回调原因。
 * @param pErrorCode 错误码。
 */
void U_CALLCONV codePageFromUCallback(
    const void* context,
    UConverterFromUnicodeArgs* fromUArgs,
    const UChar* codeUnits,
    int32_t length,
    UChar32 codePoint,
    UConverterCallbackReason reason,
    UErrorCode* pErrorCode)
{
    if (reason == UCNV_UNASSIGNED) {
        std::string* unmappable_chars_str = static_cast<std::string*>(const_cast<void*>(context));

        // 使用 U8_APPEND 宏直接将码点追加到 std::string 中
        size_t current_size = unmappable_chars_str->size();
        unmappable_chars_str->resize(current_size + U8_MAX_LENGTH); // 预留足够空间
        char* buffer_ptr = &(*unmappable_chars_str)[current_size];
        int32_t i = 0; // U8_APPEND 需要一个索引变量
        // U8_APPEND_UNSAFE 是不进行边界检查的版本，速度最快
        // 在这里我们已经确保了空间足够，所以可以使用它
        U8_APPEND_UNSAFE(buffer_ptr, i, codePoint);
        // 根据实际写入的字节数调整string的大小
        unmappable_chars_str->resize(current_size + i);
    }
    ucnv_cbFromUWriteSub(fromUArgs, 0, pErrorCode);
}

/**
 * @brief 查找一个UTF-8字符串中所有不能被转换为codePage的字符。
 *
 * @param transViewToCheck 输入的UTF-8编码的std::string。
 */
const std::string& CodePageChecker::findUnmappableChars(const std::string& transViewToCheck) {
    m_unmappableCharsResult.clear();
    UErrorCode status = U_ZERO_ERROR;
    // 目标缓冲区可以很小，因为我们不关心成功转换的结果
    std::vector<char> targetBuffer(transViewToCheck.length() + 1);

    // 正确设置ucnv_convertEx的参数
    const char* sourcePtr = transViewToCheck.c_str();
    const char* sourceLimit = sourcePtr + transViewToCheck.length();
    char* targetPtr = targetBuffer.data();
    const char* targetLimit = targetPtr + targetBuffer.size();

    ucnv_convertEx(m_codePageConverter.get(), m_u8Converter.get(),
        &targetPtr, targetLimit,
        &sourcePtr, sourceLimit,
        nullptr, nullptr, nullptr, nullptr,
        TRUE, TRUE,
        &status);

    // U_INVALID_CHAR_FOUND 错误是预期的，因为它会触发回调。
    // U_BUFFER_OVERFLOW_ERROR 也可能发生，但我们不关心，因为我们不使用目标缓冲区的结果。
    // 我们只关心其他意外的严重错误。
    if (U_FAILURE(status) && status != U_INVALID_CHAR_FOUND && status != U_BUFFER_OVERFLOW_ERROR) {
        throw std::runtime_error(std::format("ICU conversion failed with unexpected error : {}", u_errorName(status)));
    }
    return m_unmappableCharsResult;
}

void CodePageChecker::run(Sentence* se)
{
    const std::string& transViewToCheck = se->translated_preview;
    const std::string& unmappableChars = findUnmappableChars(transViewToCheck);
    if (!unmappableChars.empty()) {
        m_logger->error("字符集检查器 发现非 {} 字符: {}", m_codePage, unmappableChars);
        se->problems.push_back(std::format("非 {} 字符: {}", m_codePage, unmappableChars));
    }
}
