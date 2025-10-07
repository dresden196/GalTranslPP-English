module;

#ifdef _WIN32
#include <Windows.h>
#endif


#include <ranges>
#include <spdlog/spdlog.h>
#include <zip.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/brkiter.h>
#include <unicode/schriter.h>
#include <unicode/regex.h>
#include <unicode/uscript.h>
#include <unicode/translit.h>
#include <toml.hpp>

#include <opencc/opencc.h>
#pragma comment(lib, "../lib/marisa.lib")
#pragma comment(lib, "../lib/opencc.lib")

export module Tool;

import <nlohmann/json.hpp>;
export import std;
export import GPPDefines;
export import ITranslator;

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
namespace fs = std::filesystem;

export {

#ifdef _WIN32
    std::string wide2Ascii(const std::wstring& wide, UINT CodePage = 65001, LPBOOL usedDefaultChar = nullptr);

    std::wstring ascii2Wide(const std::string& ascii, UINT CodePage = 65001);

    std::string ascii2Ascii(const std::string& ascii, UINT src = 65001, UINT dst = 0, LPBOOL usedDefaultChar = nullptr);

    bool executeCommand(const std::wstring& program, const std::wstring& args, bool showWindow = false);

    int getConsoleWidth();
#endif

    std::string getNameString(const Sentence* se);

    std::string getNameString(const json& j);

    void createParent(const fs::path& path);

    std::wstring wstr2Lower(const std::wstring& wstr);

    std::vector<std::string> splitString(const std::string& s, char delimiter);

    std::vector<std::string> splitString(const std::string& s, const std::string& delimiter);

    std::vector<std::string> splitTsvLine(const std::string& line, const std::vector<std::string>& delimiters);

    const std::string& chooseStringRef(const Sentence* sentence, CachePart tar);

    std::string chooseString(const Sentence* sentence, CachePart tar);

    CachePart chooseCachePart(const std::string& partName);

    bool isSameExtension(const fs::path& filePath, const std::wstring& ext);

    std::string removePunctuation(const std::string& text);

    std::string removeWhitespace(const std::string& text);

    std::pair<std::string, int> getMostCommonChar(const std::string& s);

    std::vector<std::string> splitIntoGraphemes(const std::string& sourceString);

    size_t countGraphemes(const std::string& sourceString);

    int countSubstring(const std::string& text, const std::string& sub);

    std::vector<double> getSubstringPositions(const std::string& text, const std::string& sub);

    std::string& replaceStrInplace(std::string& str, const std::string& org, const std::string& rep);

    std::string extractCharactersByScripts(const std::string& sourceString, const std::vector<UScriptCode>& targetScripts);

    std::string extractKatakana(const std::string& sourceString);

    std::string extractKana(const std::string& sourceString);

    std::string extractLatin(const std::string& sourceString);

    std::string extractHangul(const std::string& sourceString);

    std::function<std::string(const std::string&)> getTraditionalChineseExtractor(std::shared_ptr<spdlog::logger> logger);

    void extractZip(const fs::path& zipPath, const fs::path& outputDir);


    struct ConditionPattern {
        CachePart conditionTarget;
        std::shared_ptr<icu::RegexPattern> conditionReg;
    };
    using GPPCondition = std::vector<ConditionPattern>;

    bool checkString(std::shared_ptr<icu::RegexPattern> conditionReg, const std::string& str);
    bool checkCondition(const GPPCondition& conditions, const Sentence* se);

    template<typename TC>
    GPPCondition createGppCondition(const toml::basic_value<TC>& conditions) {
        GPPCondition patterns;

        auto appendPatternFunc = [&](const auto& conditionTbl)
            {
                ConditionPattern pattern;
                pattern.conditionTarget = chooseCachePart(conditionTbl.at("conditionTarget").as_string());
                const std::string& conditionRegStr = conditionTbl.at("conditionReg").as_string();
                icu::UnicodeString ustr(icu::UnicodeString::fromUTF8(conditionRegStr));
                UErrorCode status = U_ZERO_ERROR;
                pattern.conditionReg = std::shared_ptr<icu::RegexPattern>(icu::RegexPattern::compile(ustr, 0, status));
                if (U_FAILURE(status)) {
                    throw std::runtime_error(std::format("Failed to compile regex pattern: {}", conditionRegStr));
                }
                patterns.push_back(pattern);
            };
        if (conditions.is_array()) {
            for (const auto& condition : conditions.as_array() 
                | std::views::filter([](const auto& condition) { return condition.is_table(); }))
            {
                appendPatternFunc(condition.as_table());
            }
        }
        else if (conditions.is_table()) {
            appendPatternFunc(conditions.as_table());
            if (conditions.contains("additionalPatterns") && conditions.at("additionalPatterns").is_array()) {
                for (const auto& condition : conditions.at("additionalPatterns").as_array() 
                    | std::views::filter([](const auto& condition) { return condition.is_table(); }))
                {
                    appendPatternFunc(condition.as_table());
                }
            }
        }
        
        return patterns;
    }

    template<typename T>
    T calculateAbs(T a, T b) {
        return a > b ? a - b : b - a;
    }

    // 辅助函数：在一个 TOML 表中，根据一个由键组成的路径向量来查找值
    template<typename TC>
    const toml::basic_value<TC>* findValueByPath(const toml::basic_value<TC>& table, const std::vector<std::string>& keys) {
        const toml::basic_value<TC>* pCurrentValue = &table;
        for (const auto& key : keys) {
            // 确保当前值是一个表 (table) 或有序表 (ordered_value)
            if (!pCurrentValue->is_table()) {
                return nullptr; // 路径中间部分不是一个表，无法继续查找
            }
            // 检查表中是否包含下一个键
            if (!pCurrentValue->contains(key)) {
                return nullptr; // 键不存在
            }
            // 移动到下一层
            pCurrentValue = &pCurrentValue->at(key);
        }
        return pCurrentValue;
    }

    template<typename T, typename TC, typename TC2>
    auto parseToml(const toml::basic_value<TC>& config, const toml::basic_value<TC2>& backup, const std::string& path) -> decltype(auto) {
        const std::vector<std::string> keys = splitString(path, '.');
        if (
            keys.empty() ||
            std::ranges::any_of(keys, [](const std::string& key) { return key.empty(); })
            ) {
            throw std::runtime_error("Invalid TOML path: " + path);
        }
        if (auto pValue = findValueByPath(config, keys)) {
            return toml::get<T>(*pValue);
        }
        if (auto pValue = findValueByPath(backup, keys)) {
            return toml::get<T>(*pValue);
        }
        throw std::runtime_error("Failed to find value in TOML: " + path);
    }

    template<typename TC,typename T>
    toml::basic_value<TC>& insertToml(toml::basic_value<TC>& table, const std::string& path, const T& value)
    {
        const std::vector<std::string> keys = splitString(path, '.');
        auto* pCurrentTable = &table.as_table();
        for (size_t i = 0; i < keys.size() - 1; i++) {
            auto& subTable = (*pCurrentTable)[keys[i]];
            if (!subTable.is_table()) {
                subTable = toml::table{};
            }
            pCurrentTable = &subTable.as_table();
        }
        auto& lastVal = (*pCurrentTable)[keys.back()];
        if constexpr (std::is_same_v<toml::basic_value<TC>, toml::ordered_value>) {
            const auto orgComments = lastVal.comments();
            lastVal = toml::ordered_value{ value };
            lastVal.comments() = orgComments;
        }
        else {
            const auto orgComments = lastVal.comments();
            lastVal = toml::value{ value };
            lastVal.comments() = orgComments;
        }
        return table;
    }

}



module :private;

#ifdef _WIN32
std::string wide2Ascii(const std::wstring& wide, UINT CodePage, LPBOOL usedDefaultChar) {
    int len = WideCharToMultiByte
    (CodePage, 0, wide.c_str(), -1, nullptr, 0, nullptr, usedDefaultChar);
    if (len == 0) return {};
    std::string ascii(len, '\0');
    WideCharToMultiByte
    (CodePage, 0, wide.c_str(), -1, &ascii[0], len, nullptr, nullptr);
    ascii.pop_back();
    return ascii;
}

std::wstring ascii2Wide(const std::string& ascii, UINT CodePage) {
    int len = MultiByteToWideChar(CodePage, 0, ascii.c_str(), -1, nullptr, 0);
    if (len == 0) return {};
    std::wstring wide(len, L'\0');
    MultiByteToWideChar(CodePage, 0, ascii.c_str(), -1, &wide[0], len);
    wide.pop_back();
    return wide;
}

std::string ascii2Ascii(const std::string& ascii, UINT src, UINT dst, LPBOOL usedDefaultChar) {
    return wide2Ascii(ascii2Wide(ascii, src), dst, usedDefaultChar);
}

bool executeCommand(const std::wstring& program, const std::wstring& args, bool showWindow) {

    // 1. 构造完整的命令行字符串。
    //    CreateProcess 的 lpCommandLine 参数要求，如果程序路径包含空格，
    //    它应该被双引号包围。
    std::wstring commandLineStr = L"\"" + program + L"\" " + args;

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.dwFlags |= STARTF_USESHOWWINDOW; // 指定 wShowWindow 成员有效
    si.wShowWindow = showWindow ? SW_SHOW : SW_HIDE;          // 将窗口设置为隐藏
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcessW 需要一个可写的字符缓冲区
    std::vector<wchar_t> commandLineVec(commandLineStr.begin(), commandLineStr.end());
    commandLineVec.push_back(L'\0');

    //    设置 dwCreationFlags
    //    CREATE_NEW_CONSOLE 会为新进程创建一个新的控制台窗口。
    //    这个新窗口会显示 python.exe 的所有标准输出和标准错误。
    DWORD creationFlags = 0;
    if (showWindow) {
        creationFlags |= CREATE_NEW_CONSOLE;
    }

    if (!CreateProcessW(NULL,           // lpApplicationName
        &commandLineVec[0], // lpCommandLine (must be writable)
        NULL,           // lpProcessAttributes
        NULL,           // lpThreadAttributes
        FALSE,          // bInheritHandles
        creationFlags,  // dwCreationFlags
        NULL,           // lpEnvironment
        NULL,           // lpCurrentDirectory
        &si,            // lpStartupInfo
        &pi)) {         // lpProcessInformation
        return false;
    }

    // 等待子进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 清理句柄
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) {
        return 80;
    }
    // 获取控制台屏幕缓冲区信息
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        // 宽度 = 右坐标 - 左坐标 + 1
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80; // 获取失败，返回默认值
}
#endif

std::string names2String(const std::vector<std::string>& names) {
    std::string result;
    for (const auto& name : names) {
        result += name + "|";
    }
    if (!result.empty()) {
        result.pop_back();
    }
    return result;
}

std::string names2String(const json& j) {
    std::string result;
    for (const auto& name : j) {
        result += name.get<std::string>() + "|";
    }
    if (!result.empty()) {
        result.pop_back();
    }
    return result;
}

std::string getNameString(const Sentence* se) {
    if (se->nameType == NameType::Single) {
        return se->name;
    }
    if (se->nameType == NameType::Multiple) {
        return names2String(se->names);
    }
    return {};
}

std::string getNameString(const json& j) {
    if (j.contains("name")) {
        return j["name"].get<std::string>();
    }
    if (j.contains("names")) {
        return names2String(j["names"]);
    }
    return {};
}

void createParent(const fs::path& path) {
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
}

std::wstring wstr2Lower(const std::wstring& wstr) {
    std::wstring result = wstr;
    std::ranges::transform(result, result.begin(), [](wchar_t wc) { return std::tolower(wc); });
    return result;
}

std::vector<std::string> splitString(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

std::vector<std::string> splitString(const std::string& s, char delimiter) {
    return splitString(s, std::string(1, delimiter));
}

std::vector<std::string> splitTsvLine(const std::string& line, const std::vector<std::string>& delimiters) {
    std::vector<std::string> parts;
    size_t currentPos = 0;

    if (
        std::ranges::any_of(delimiters, [](const std::string& delimiter)
            {
                return delimiter.empty();
            })
        )
    {
        throw std::runtime_error("Empty delimiter is not allowed in TSV line splitting");
    }

    while (currentPos < line.length()) {
        size_t splitPos = std::string::npos;
        size_t delimiterLength = 0;
        for (const auto& delimiter : delimiters) {
            size_t pos = line.find(delimiter, currentPos);
            if (pos != std::string::npos && pos < splitPos) {
                splitPos = pos;
                delimiterLength = delimiter.length();
            }
        }
        std::string part = line.substr(currentPos, splitPos - currentPos);
        parts.push_back(part);

        if (splitPos == std::string::npos) {
            break;
        }
        currentPos = splitPos + delimiterLength;
    }

    return parts;
}


const std::string& chooseStringRef(const Sentence* sentence, CachePart tar) {
    switch (tar) {
    case CachePart::Name:
        return sentence->name;
        break;
    case CachePart::NamePreview:
        return sentence->name_preview;
        break;
    case CachePart::OrigText:
        return sentence->original_text;
        break;
    case CachePart::PreprocText:
        return sentence->pre_processed_text;
        break;
    case CachePart::PretransText:
        return sentence->pre_translated_text;
        break;
    case CachePart::TranslatedBy:
        return sentence->translated_by;
        break;
    case CachePart::TransPreview:
        return sentence->translated_preview;
        break;
    case CachePart::None:
        throw std::runtime_error("Invalid condition target: None");
    default:
        throw std::runtime_error("Invalid condition target to get string: " + std::to_string((int)tar));
    }
    return {};
}

std::string chooseString(const Sentence* sentence, CachePart tar) {
    return chooseStringRef(sentence, tar);
}

CachePart chooseCachePart(const std::string& partName) {
    CachePart part = CachePart::None;
    if (partName == "name") {
        part = CachePart::Name;
    }
    else if (partName == "names") {
        part = CachePart::Names;
    }
    else if (partName == "name_preview") {
        part = CachePart::NamePreview;
    }
    else if (partName == "names_preview") {
        part = CachePart::NamesPreview;
    }
    else if (partName == "orig_text") {
        part = CachePart::OrigText;
    }
    else if (partName == "preproc_text") {
        part = CachePart::PreprocText;
    }
    else if (partName == "pretrans_text") {
        part = CachePart::PretransText;
    }
    else if (partName == "problems") {
        part = CachePart::Problems;
    }
    else if (partName == "other_info") {
        part = CachePart::OtherInfo;
    }
    else if (partName == "translated_by") {
        part = CachePart::TranslatedBy;
    }
    else if (partName == "trans_preview") {
        part = CachePart::TransPreview;
    }
    else {
        throw std::invalid_argument("无效的 CachePart: " + partName);
    }
    return part;
}

bool isSameExtension(const fs::path& filePath, const std::wstring& ext) {
    return _wcsicmp(filePath.extension().wstring().c_str(), ext.c_str()) == 0;
}


std::string removePunctuation(const std::string& text) {
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(text);
    icu::UnicodeString resultUstr;
    icu::StringCharacterIterator iter(ustr);
    UChar32 codePoint;
    while (iter.hasNext()) {
        codePoint = iter.next32PostInc();
        if (!u_ispunct(codePoint)) {
            resultUstr.append(codePoint);
        }
    }
    std::string utf8Output;
    return resultUstr.toUTF8String(utf8Output);
}

std::string removeWhitespace(const std::string& text) {
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(text);
    icu::UnicodeString resultUstr;
    icu::StringCharacterIterator iter(ustr);
    UChar32 codePoint;
    while (iter.hasNext()) {
        codePoint = iter.next32PostInc();
        if (!u_isspace(codePoint)) {
            resultUstr.append(codePoint);
        }
    }
    std::string utf8Output;
    return resultUstr.toUTF8String(utf8Output);
}

// 这两个函数遍历字形簇
std::pair<std::string, int> getMostCommonChar(const std::string& s) {
    if (s.empty()) {
        return { {}, 0 };
    }

    UErrorCode errorCode = U_ZERO_ERROR;
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(s);

    std::map<icu::UnicodeString, int> counts;

    std::unique_ptr<icu::BreakIterator> boundary(icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(), errorCode));
    if (U_FAILURE(errorCode)) {
        throw std::runtime_error(std::format("Failed to create a character break iterator: {}", u_errorName(errorCode)));
    }
    boundary->setText(ustr);

    int32_t start = boundary->first();
    for (int32_t end = boundary->next(); end != icu::BreakIterator::DONE; start = end, end = boundary->next()) {
        icu::UnicodeString grapheme;
        ustr.extract(start, end - start, grapheme);
        counts[grapheme]++;
    }

    if (counts.empty()) {
        return { {}, 0 };
    }

    auto maxIterator = std::max_element(counts.begin(), counts.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    icu::UnicodeString mostCommonGrapheme = maxIterator->first;
    int maxCount = maxIterator->second;

    std::string resultStr;

    return { mostCommonGrapheme.toUTF8String(resultStr), maxCount };
}

std::vector<std::string> splitIntoGraphemes(const std::string& sourceString) {
    std::vector<std::string> resultVector;

    if (sourceString.empty()) {
        return resultVector;
    }

    UErrorCode errorCode = U_ZERO_ERROR;
    icu::UnicodeString uString = icu::UnicodeString::fromUTF8(sourceString);

    std::unique_ptr<icu::BreakIterator> breakIterator(
        icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(), errorCode)
    );

    if (U_FAILURE(errorCode)) {
        throw std::runtime_error(std::format("Failed to create a character break iterator: {}", u_errorName(errorCode)));
    }

    breakIterator->setText(uString);

    int32_t start = breakIterator->first();
    for (int32_t end = breakIterator->next(); end != icu::BreakIterator::DONE; start = end, end = breakIterator->next()) {
        icu::UnicodeString graphemeUString;
        uString.extract(start, end - start, graphemeUString);
        std::string graphemeStdString;
        resultVector.push_back(graphemeUString.toUTF8String(graphemeStdString));
    }

    return resultVector;
}

size_t countGraphemes(const std::string& sourceString) {
    if (sourceString.empty()) {
        return 0;
    }
    UErrorCode errorCode = U_ZERO_ERROR;
    icu::UnicodeString uString = icu::UnicodeString::fromUTF8(sourceString);
    std::unique_ptr<icu::BreakIterator> breakIterator(
        icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(), errorCode)
    );
    if (U_FAILURE(errorCode)) {
        throw std::runtime_error(std::format("Failed to create a character break iterator: {}", u_errorName(errorCode)));
    }
    breakIterator->setText(uString);
    int32_t start = breakIterator->first();
    size_t count = 0;
    while (start != icu::BreakIterator::DONE) {
        ++count;
        start = breakIterator->next();
    }
    return count;
}

// 计算子串出现次数
int countSubstring(const std::string& text, const std::string& sub) {
    if (sub.empty()) return 0;
    int count = 0;
    for (size_t offset = text.find(sub); offset != std::string::npos; offset = text.find(sub, offset + sub.length())) {
        ++count;
    }
    return count;
}

// 计算的是子串在删去子串后的主串中出现的位置
std::vector<double> getSubstringPositions(const std::string& text, const std::string& sub) {
    if (text.empty() || sub.empty()) return {};
    std::vector<size_t> positions;
    std::vector<double> relpositions;

    for (size_t offset = text.find(sub); offset != std::string::npos; offset = text.find(sub, offset + sub.length())) {
        positions.push_back(offset);
    }
    size_t newTotalLength = text.length() - positions.size() * sub.length();
    if (newTotalLength == 0) {
        return {};
    }
    for (size_t i = 0; i < positions.size(); i++) {
        size_t newPos = positions[i] - i * sub.length();
        relpositions.push_back((double)newPos / newTotalLength);
    }
    return relpositions;
}

std::string& replaceStrInplace(std::string& str, const std::string& org, const std::string& rep) {
    size_t pos = 0;
    while ((pos = str.find(org, pos)) != std::string::npos) {
        str = str.replace(pos, org.length(), rep);
        pos += rep.length();
    }
    return str;
}

// 核心辅助函数
// 接受一个源字符串和一组目标脚本，返回所有匹配字符组成的UTF-8字符串
std::string extractCharactersByScripts(const std::string& sourceString, const std::vector<UScriptCode>& targetScripts) {

    icu::UnicodeString resultUString;
    icu::UnicodeString sourceUString = icu::UnicodeString::fromUTF8(sourceString);

    icu::StringCharacterIterator iter(sourceUString);
    UChar32 codePoint;
    UErrorCode errorCode = U_ZERO_ERROR;

    while (iter.hasNext()) {
        codePoint = iter.next32PostInc();
        UScriptCode script = uscript_getScript(codePoint, &errorCode);

        if (U_SUCCESS(errorCode)) {
            auto it = std::ranges::find(targetScripts, script);
            if (it != targetScripts.end()) {
                resultUString.append(codePoint);
            }
        }
    }

    std::string resultString;
    return resultUString.toUTF8String(resultString);
}

std::string extractKatakana(const std::string& sourceString) {
    return extractCharactersByScripts(sourceString, { USCRIPT_KATAKANA });
}

std::string extractKana(const std::string& sourceString) {
    return extractCharactersByScripts(sourceString, { USCRIPT_HIRAGANA, USCRIPT_KATAKANA });
}

std::string extractLatin(const std::string& sourceString) {
    return extractCharactersByScripts(sourceString, { USCRIPT_LATIN });
}

std::string extractHangul(const std::string& sourceString) {
    return extractCharactersByScripts(sourceString, { USCRIPT_HANGUL });
}

std::function<std::string(const std::string&)> getTraditionalChineseExtractor(std::shared_ptr<spdlog::logger> logger)
{
    // 是否需要线程安全？(似乎是不需要)
    std::function<std::string(const std::string&)> result;
    try {
        std::shared_ptr<opencc::SimpleConverter> converter = std::make_shared<opencc::SimpleConverter>("BaseConfig/opencc/t2s.json");
        result = [=](const std::string& sourceString)
            {
                static const std::set<std::string> excludeList =
                {
                    //"乾", "阪",
                };
                std::string resultStr;
                std::vector<std::string> graphemes = splitIntoGraphemes(sourceString);
                for (const auto& grapheme : graphemes | std::views::filter([&](const std::string& g) { return !excludeList.contains(g); })) {
                    std::string simplified = converter->Convert(grapheme);
                    if (simplified != grapheme) {
                        resultStr += grapheme;
                    }
                }
                return resultStr;
            };
        logger->info("OpenCC is usable for traditional Chinese conversion");
        return result;
    }
    catch (...) {
        logger->error("OpenCC is not usable, falling back to ICU-based traditional Chinese conversion");
        UErrorCode status = U_ZERO_ERROR;
        auto toSimplified = std::shared_ptr<icu::Transliterator>(icu::Transliterator::createInstance("Traditional-Simplified", UTRANS_FORWARD, status));
        if (U_FAILURE(status)) {
            throw std::runtime_error("ICU-based traditional Chinese conversion is not available");
        }
        auto toTraditional = std::shared_ptr<icu::Transliterator>(icu::Transliterator::createInstance("Simplified-Traditional", UTRANS_FORWARD, status));
        if (U_FAILURE(status)) {
            throw std::runtime_error("ICU-based simplified Chinese conversion is not available");
        }

        result = [=](const std::string& sourceString)
            {
                // 白名单/排除列表：用于解决简繁转换中的歧义问题。
                // "著" (U+8457) 是一个典型例子，它在简体中文里也是合法字符，但T->S的转换规则可能导致误判。
                static const std::set<UChar32> excludeList = {
                    0x8457 // 著
                };

                icu::UnicodeString uSource = icu::UnicodeString::fromUTF8(sourceString);
                std::set<UChar32> traditionalChars;

                icu::StringCharacterIterator iter(uSource);
                UChar32 charSource;
                while (iter.hasNext()) {
                    charSource = iter.next32PostInc();

                    // 1. 必须是汉字
                    UErrorCode scriptErr = U_ZERO_ERROR;
                    if (uscript_getScript(charSource, &scriptErr) != USCRIPT_HAN || U_FAILURE(scriptErr)) {
                        continue;
                    }

                    // 2. 检查是否在排除列表中
                    if (excludeList.contains(charSource)) {
                        continue;
                    }

                    icu::UnicodeString uCharSource(charSource);
                    icu::UnicodeString uSimplified = uCharSource;
                    toSimplified->transliterate(uSimplified);

                    // 3. 繁体转简体后必须有变化
                    if (uCharSource == uSimplified) {
                        continue;
                    }

                    // 4. 核心双向检查：简体转回繁体，必须能得到原始字符，确保转换是明确且可逆的
                    icu::UnicodeString uReTraditional = uSimplified;
                    toTraditional->transliterate(uReTraditional);

                    if (uReTraditional == uCharSource) {
                        traditionalChars.insert(charSource);
                    }
                }

                icu::UnicodeString resultUStr;
                for (UChar32 ch : traditionalChars) {
                    resultUStr.append(ch);
                }

                std::string resultStr;
                return resultUStr.toUTF8String(resultStr);
            };
        logger->info("ICU-based traditional Chinese conversion is available");
        return result;
    }
    return result;
}


void extractZip(const fs::path& zipPath, const fs::path& outputDir) {
    int error = 0;
    zip* za = zip_open(wide2Ascii(zipPath).c_str(), 0, &error);
    if (!za) {
        throw std::runtime_error(std::format("Failed to open zip archive: {}", wide2Ascii(zipPath)));
    }
    zip_int64_t numEntries = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < numEntries; i++) {
        zip_stat_t zs;
        zip_stat_index(za, i, 0, &zs);
        fs::path path = outputDir / ascii2Wide(zs.name);
        if (zs.name[strlen(zs.name) - 1] == '/') {
            fs::create_directories(path);
        }
        else {
            zip_file* zf = zip_fopen_index(za, i, 0);
            if (!zf) {
                throw std::runtime_error(std::format("Failed to open file in zip archive: {}, file: {}", wide2Ascii(zipPath), zs.name));
            }
            std::vector<char> buffer(zs.size);
            zip_fread(zf, buffer.data(), zs.size);
            zip_fclose(zf);
            fs::create_directories(path.parent_path());
            std::ofstream ofs(path, std::ios::binary);
            ofs.write(buffer.data(), buffer.size());
        }
    }
    zip_close(za);
}

bool checkString(std::shared_ptr<icu::RegexPattern> conditionReg, const std::string& str) {
    icu::UnicodeString textToInspect = icu::UnicodeString::fromUTF8(str);
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::RegexMatcher> matcher(conditionReg->matcher(textToInspect, status));
    if (U_FAILURE(status)) {
        std::string textToInspectU8;
        throw std::runtime_error(std::format("正则表达式创建matcher失败: {}, 句子: [{}]", u_errorName(status), textToInspect.toUTF8String(textToInspectU8)));
    }
    return (bool)matcher->find();
}

bool checkCondition(const GPPCondition& conditions, const Sentence* se) {
    return std::ranges::all_of(conditions, [&](const ConditionPattern& pattern)
        {
            auto checkAnyOf = [&]<typename ContainerType>(const ContainerType & container) -> bool
            {
                return std::ranges::any_of(container, [&](const auto& item)
                    {
                        if constexpr (std::is_same_v<ContainerType, std::map<std::string, std::string>>) {
                            return checkString(pattern.conditionReg, item.second);
                        }
                        else {
                            return checkString(pattern.conditionReg, item);
                        }
                    });
            };
            switch (pattern.conditionTarget) {
            case CachePart::Names:
                return checkAnyOf(se->names);
            case CachePart::NamesPreview:
                return checkAnyOf(se->names_preview);
            case CachePart::Problems:
                return checkAnyOf(se->problems);
            case CachePart::OtherInfo:
                return checkAnyOf(se->other_info);
            default:
                return checkString(pattern.conditionReg, chooseStringRef(se, pattern.conditionTarget));
            }
            return false;
        });
}


template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const std::string& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const int& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const double& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const bool& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const toml::array& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const toml::table& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const toml::value& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const toml::ordered_array& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const toml::ordered_table& value);
template toml::ordered_value& insertToml(toml::ordered_value& table, const std::string& path, const toml::ordered_value& value);