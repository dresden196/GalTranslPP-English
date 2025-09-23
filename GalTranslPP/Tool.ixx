module;

#include <Windows.h>
#include <spdlog/spdlog.h>
#include <zip.h>
#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <unicode/brkiter.h>
#include <unicode/schriter.h>
#include <unicode/uscript.h>

export module Tool;

import <nlohmann/json.hpp>;
import <toml++/toml.hpp>;
export import std;
export import GPPDefines;
export import ITranslator;

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
namespace fs = std::filesystem;

export {

    std::string wide2Ascii(const std::wstring& wide, UINT CodePage = 65001, LPBOOL usedDefaultChar = nullptr) {
        int len = WideCharToMultiByte
        (CodePage, 0, wide.c_str(), -1, nullptr, 0, nullptr, usedDefaultChar);
        if (len == 0) return {};
        std::string ascii(len, '\0');
        WideCharToMultiByte
        (CodePage, 0, wide.c_str(), -1, &ascii[0], len, nullptr, nullptr);
        ascii.pop_back();
        return ascii;
    }

    std::wstring ascii2Wide(const std::string& ascii, UINT CodePage = 65001) {
        int len = MultiByteToWideChar(CodePage, 0, ascii.c_str(), -1, nullptr, 0);
        if (len == 0) return {};
        std::wstring wide(len, L'\0');
        MultiByteToWideChar(CodePage, 0, ascii.c_str(), -1, &wide[0], len);
        wide.pop_back();
        return wide;
    }

    std::string ascii2Ascii(const std::string& ascii, UINT src, UINT dst, LPBOOL usedDefaultChar = nullptr) {
        return wide2Ascii(ascii2Wide(ascii, src), dst, usedDefaultChar);
    }

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

    std::string getNameString(const json& j) {
        if (j.contains("name")) {
            return j["name"].get<std::string>();
        }
        else if (j.contains("names")) {
            return names2String(j["names"]);
        }
        return {};
    }

    std::string getNameString(const Sentence* se) {
        if (se->nameType == NameType::Single) {
            return se->name;
        }
        else if (se->nameType == NameType::Multiple) {
            return names2String(se->names);
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
        std::transform(result.begin(), result.end(), result.begin(), [](wchar_t wc) { return std::tolower(wc); });
        return result;
    }

    std::vector<std::string> splitString(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::vector<std::string> splitString(const std::string& s, const std::string& delimiter) {
        std::vector<std::string> parts;
        for (const auto& part_view : std::views::split(s, delimiter)) {
            parts.emplace_back(part_view.begin(), part_view.end());
        }
        return parts;
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
        case CachePart::TransPreview:
            return sentence->translated_preview;
            break;
        case CachePart::None:
            throw std::runtime_error("Invalid condition target: None");
        default:
            throw std::runtime_error("Invalid condition target");
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
        else if (partName == "name_preview") {
            part = CachePart::NamePreview;
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
        else if (partName == "trans_preview") {
            part = CachePart::TransPreview;
        }
        return part;
    }


    template<typename T>
    T parseToml(toml::v3::table& config, toml::v3::table& backup, const std::string& path) {
        if constexpr (std::is_same_v<T, toml::array>) {
            auto arr = config.at_path(path).as_array();
            if (!arr) {
                arr = backup.at_path(path).as_array();
            }
            if (!arr) {
                throw std::runtime_error(std::format("配置项 {} 未找到", path));
            }
            return *arr;
        }
        else if constexpr (std::is_same_v<T, toml::table>) {
            auto tbl = config.at_path(path).as_table();
            if (!tbl) {
                tbl = backup.at_path(path).as_table();
            }
            if (!tbl) {
                throw std::runtime_error(std::format("配置项 {} 未找到", path));
            }
            return *tbl;
        }
        else {
            auto optValue = config.at_path(path).value<T>();
            if (!optValue.has_value()) {
                optValue = backup.at_path(path).value<T>();
            }
            if (!optValue.has_value()) {
                throw std::runtime_error(std::format("配置项 {} 未找到", path));
            }
            return optValue.value();
        }
    }


    bool isSameExtension(const fs::path& filePath, const std::wstring& ext) {
        return _wcsicmp(filePath.extension().wstring().c_str(), ext.c_str()) == 0;
    }

    std::string removePunctuation(const std::string& text) {
        UErrorCode errorCode = U_ZERO_ERROR;
        icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(text);
        icu::UnicodeString result;

        // 创建一个用于遍历字形簇的 BreakIterator
        std::unique_ptr<icu::BreakIterator> boundary(icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(), errorCode));
        if (U_FAILURE(errorCode)) {
            throw std::runtime_error(std::format("Failed to create a character break iterator: {}", u_errorName(errorCode)));
        }
        boundary->setText(ustr);

        int32_t start = boundary->first();
        // 遍历每个字形簇
        for (int32_t end = boundary->next(); end != icu::BreakIterator::DONE; start = end, end = boundary->next()) {
            icu::UnicodeString grapheme;
            ustr.extract(start, end - start, grapheme); // 提取一个完整的字形簇
            // 判断这个字形簇是否是标点
            // 一个标点符号通常自身就是一个单独的码点构成的字形簇。
            // 如果一个字形簇包含多个码点（如 'e' + '´'），它几乎不可能是标点。
            bool isPunctuationCluster = (grapheme.length() == 1 && u_ispunct(grapheme.char32At(0)));
            if (!isPunctuationCluster) {
                result.append(grapheme); // 如果不是标点，则保留整个字形簇
            }
        }

        std::string resultStr;
        result.toUTF8String(resultStr);
        return resultStr;
    }

    std::pair<std::string, int> getMostCommonChar(const std::string& s) {
        if (s.empty()) {
            return { {}, 0};
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
        mostCommonGrapheme.toUTF8String(resultStr);

        return { resultStr, maxCount };
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
            graphemeUString.toUTF8String(graphemeStdString);
            resultVector.push_back(graphemeStdString);
        }

        return resultVector;
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

    void replaceStrInplace(std::string& str, const std::string& org, const std::string& rep) {
        size_t pos = 0;
        while ((pos = str.find(org, pos)) != std::string::npos) {
            str = str.replace(pos, org.length(), rep);
            pos += rep.length();
        }
    }

    template<typename T>
    std::string stream2String(const T& output) {
        std::stringstream ss;
        ss << output;
        return ss.str();
    }
    template std::string stream2String(const toml::table& output);

    template<typename T>
    T calculateAbs(T a, T b) {
        return a > b ? a - b : b - a;
    }

    bool containsKatakana(const std::string& sourceString) {
        icu::UnicodeString uString = icu::UnicodeString::fromUTF8(sourceString);

        icu::StringCharacterIterator iter(uString);
        UChar32 codePoint;

        // 遍历字符串中的每一个码点
        while (iter.hasNext()) {
            codePoint = iter.next32PostInc(); // 获取当前码点并前进

            UErrorCode errorCode = U_ZERO_ERROR;
            // 使用 uscript_getScript 获取码点的脚本属性
            UScriptCode script = uscript_getScript(codePoint, &errorCode);

            // 检查是否获取脚本失败，或者脚本是片假名
            if (U_SUCCESS(errorCode) && script == USCRIPT_KATAKANA) {
                return true;
            }
        }

        return false;
    }

    bool containsKana(const std::string& sourceString) {
        icu::UnicodeString uString = icu::UnicodeString::fromUTF8(sourceString);
        icu::StringCharacterIterator iter(uString);
        UChar32 codePoint;
        while (iter.hasNext()) {
            codePoint = iter.next32PostInc();
            UErrorCode errorCode = U_ZERO_ERROR;
            UScriptCode script = uscript_getScript(codePoint, &errorCode);
            if (U_SUCCESS(errorCode) && (script == USCRIPT_KATAKANA || script == USCRIPT_HIRAGANA)) {
                return true;
            }
        }
        return false;
    }

    bool containsLatin(const std::string& sourceString) {
        icu::UnicodeString uString = icu::UnicodeString::fromUTF8(sourceString);
        icu::StringCharacterIterator iter(uString);
        UChar32 codePoint;
        while (iter.hasNext()) {
            codePoint = iter.next32PostInc();
            UErrorCode errorCode = U_ZERO_ERROR;
            UScriptCode script = uscript_getScript(codePoint, &errorCode);
            if (U_SUCCESS(errorCode) && script == USCRIPT_LATIN) {
                return true;
            }
        }
        return false;
    }

    bool containsHangul(const std::string& sourceString) {
        icu::UnicodeString uString = icu::UnicodeString::fromUTF8(sourceString);
        icu::StringCharacterIterator iter(uString);
        UChar32 codePoint;
        while (iter.hasNext()) {
            codePoint = iter.next32PostInc();
            UErrorCode errorCode = U_ZERO_ERROR;
            UScriptCode script = uscript_getScript(codePoint, &errorCode);
            if (U_SUCCESS(errorCode) && script == USCRIPT_HANGUL) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    void insertToml(toml::table& table, const std::string& path, const T& value)
    {
        std::vector<std::string> keys = splitString(path, '.');
        toml::table* currentTable = &table;
        for (size_t i = 0; i < keys.size() - 1; i++) {
            auto it = currentTable->find(keys[i]);
            if (it == currentTable->end()) {
                currentTable->insert(keys[i], toml::table{});
            }
            currentTable = (*currentTable)[keys[i]].as_table();
            if (!currentTable) {
                return;
            }
        }
        currentTable->insert_or_assign(keys.back(), value);
    }

    template void insertToml(toml::table& table, const std::string& path, const std::string& value);
    template void insertToml(toml::table& table, const std::string& path, const int& value);
    template void insertToml(toml::table& table, const std::string& path, const double& value);
    template void insertToml(toml::table& table, const std::string& path, const bool& value);
    template void insertToml(toml::table& table, const std::string& path, const toml::array& value);
    template void insertToml(toml::table& table, const std::string& path, const toml::table& value);

    bool extractZip(const fs::path& zipPath, const fs::path& outputDir) {
        int error = 0;
        zip* za = zip_open(wide2Ascii(zipPath).c_str(), 0, &error);
        if (!za) {
            return false;
        }
        zip_int64_t num_entries = zip_get_num_entries(za, 0);
        for (zip_int64_t i = 0; i < num_entries; i++) {
            zip_stat_t zs;
            zip_stat_index(za, i, 0, &zs);
            fs::path outpath = outputDir / ascii2Wide(zs.name);
            if (zs.name[strlen(zs.name) - 1] == '/') {
                fs::create_directories(outpath);
            }
            else {
                zip_file* zf = zip_fopen_index(za, i, 0);
                if (!zf) {
                    throw std::runtime_error(std::format("Failed to open file in zip archive: {}, file: {}", wide2Ascii(zipPath), zs.name));
                }
                std::vector<char> buffer(zs.size);
                zip_fread(zf, buffer.data(), zs.size);
                zip_fclose(zf);
                fs::create_directories(outpath.parent_path());
                std::ofstream ofs(outpath, std::ios::binary);
                ofs.write(buffer.data(), buffer.size());
            }
        }
        zip_close(za);
        return true;
    }

}