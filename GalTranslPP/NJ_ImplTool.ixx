module;

#define _RANGES_
#include <spdlog/spdlog.h>
#include <toml.hpp>
#include <sol/sol.hpp>
#include <unicode/regex.h>
#include <unicode/unistr.h>
#include <nlohmann/json.hpp>

export module NJ_ImplTool;

export import IPlugin;
export import Tool;
export import LuaManager;

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
namespace fs = std::filesystem;

export {

    std::vector<std::shared_ptr<IPlugin>> registerPlugins(const std::vector<std::string>& pluginNames, const fs::path& projectDir,
        LuaManager& luaManager, std::shared_ptr<spdlog::logger> logger,
        const toml::value& projectConfig);

    std::string generateCacheKey(const Sentence* s);

    std::string buildContextHistory(const std::vector<Sentence*>& batch, TransEngine transEngine, int contextHistorySize, int maxTokens);

    void fillBlockAndMap(const std::vector<Sentence*>& batchToTransThisRound, std::map<int, Sentence*>& id2SentenceMap, std::string& inputBlock, TransEngine transEngine);

    void parseContent(std::string& content, std::vector<Sentence*>& batchToTransThisRound, std::map<int, Sentence*>& id2SentenceMap, const std::string& modelName,
        TransEngine transEngine, bool& parseError, int& parsedCount, std::shared_ptr<IController> controller, std::atomic<int>& completedSentences);

    void combineOutputFiles(const fs::path& originalRelFilePath, const std::map<fs::path, bool>& splitFileParts,
        std::shared_ptr<spdlog::logger> logger, const fs::path& outputCacheDir, const fs::path& outputDir);

    bool hasRetranslKey(const std::vector<CheckSeCondFunc>& retranslKeys, const json& item, const Sentence* currentSe);

    void saveCache(const std::vector<Sentence>& allSentences, const fs::path& cachePath);

    std::vector<json> splitJsonArrayNum(const json& originalData, int chunkSize);

    std::vector<json> splitJsonArrayEqual(const json& originalData, int numParts);
}



module :private;

/**
* @brief 根据句子的上下文生成唯一的缓存键，复刻 GalTransl 逻辑
*/
std::string generateCacheKey(const Sentence* s) {
    std::string prev_text = "None";
    const Sentence* temp = s->prev;
    if (temp) {
        prev_text = getNameString(temp) + temp->original_text + temp->pre_processed_text;
    }

    std::string current_text = getNameString(s) + s->original_text + s->pre_processed_text;

    std::string next_text = "None";
    temp = s->next;
    if (temp) {
        next_text = getNameString(temp) + temp->original_text + temp->pre_processed_text;
    }

    return prev_text + current_text + next_text;
}

std::string buildContextHistory(const std::vector<Sentence*>& batch, TransEngine transEngine, int contextHistorySize, int maxTokens) {
    if (batch.empty() || !batch[0]->prev) {
        return {};
    }

    std::string history;

    switch (transEngine) {
    case TransEngine::ForGalTsv: {
        history = "NAME\tDST\tID\n"; // Or DST\tID for novel
        std::vector<std::string> contextLines;
        const Sentence* current = batch[0]->prev;
        int count = 0;
        while (current && count < contextHistorySize) {
            if (current->complete) {
                std::string name = current->nameType == NameType::None ? "null" : getNameString(current);
                contextLines.push_back(name + "\t" + current->pre_translated_text + "\t" + std::to_string(current->index));
                count++;
            }
            current = current->prev;
        }
        std::reverse(contextLines.begin(), contextLines.end());
        if (contextLines.empty()) return {};
        for (const auto& line : contextLines) {
            history += line + "\n";
        }
        break;
    }
    case TransEngine::ForNovelTsv: {
        history = "DST\tID\n"; // Or DST\tID for novel
        std::vector<std::string> contextLines;
        const Sentence* current = batch[0]->prev;
        int count = 0;
        while (current && count < contextHistorySize) {
            if (current->complete) {
                contextLines.push_back(current->pre_translated_text + "\t" + std::to_string(current->index));
                count++;
            }
            current = current->prev;
        }
        std::reverse(contextLines.begin(), contextLines.end());
        if (contextLines.empty()) return {};
        for (const auto& line : contextLines) {
            history += line + "\n";
        }
        break;
    }

    case TransEngine::ForGalJson:
    case TransEngine::DeepseekJson:
    {
        json historyJson = json::array();
        const Sentence* current = batch[0]->prev;
        int count = 0;
        while (current && count < contextHistorySize) {
            if (current->complete) { // current->complete
                json item;
                item["id"] = current->index;
                if (current->nameType != NameType::None) {
                    item["name"] = getNameString(current);
                }
                item["dst"] = current->pre_translated_text;
                historyJson.push_back(item);
                count++;
            }
            current = current->prev;
        }
        std::reverse(historyJson.begin(), historyJson.end());
        for (const auto& item : historyJson) {
            history += item.dump() + "\n";
        }
        history = "```jsonline\n" + history + "```";
    }
    break;

    case TransEngine::Sakura:
    {
        const Sentence* current = batch[0]->prev;
        int count = 0;
        std::vector<std::string> contextLines;
        while (current && count < contextHistorySize) {
            if (current->complete) {
                if (current->nameType != NameType::None) {
                    contextLines.push_back(getNameString(current) + ":::::" + current->pre_translated_text); // :::::
                }
                else {
                    contextLines.push_back(current->pre_translated_text);
                }
                count++;
            }
            current = current->prev;
        }
        std::reverse(contextLines.begin(), contextLines.end());
        for (const auto& line : contextLines) {
            history += line + "\n";
        }
    }
    break;
    default:
        throw std::runtime_error("未知的 PromptType");
    }

    // 缺tiktoken这一块
    if (history.length() > maxTokens) {
        history = history.substr(history.length() - maxTokens);
    }
    return history;
}

void fillBlockAndMap(const std::vector<Sentence*>& batchToTransThisRound, std::map<int, Sentence*>& id2SentenceMap, std::string& inputBlock, TransEngine transEngine) {
    switch (transEngine) {
    case TransEngine::ForGalTsv: {
        for (const auto& pSentence : batchToTransThisRound) {
            std::string name = pSentence->nameType == NameType::None ? "null" : getNameString(pSentence);
            inputBlock += name + "\t" + pSentence->pre_processed_text + "\t" + std::to_string(pSentence->index) + "\n";
            id2SentenceMap[pSentence->index] = pSentence;
        }
        break;
    }
    case TransEngine::ForNovelTsv: {
        for (const auto& pSentence : batchToTransThisRound) {
            inputBlock += pSentence->pre_processed_text + "\t" + std::to_string(pSentence->index) + "\n";
            id2SentenceMap[pSentence->index] = pSentence;
        }
        break;
    }

    case TransEngine::ForGalJson:
    case TransEngine::DeepseekJson:
        for (const auto& pSentence : batchToTransThisRound) {
            json item;
            item["id"] = pSentence->index;
            if (pSentence->nameType != NameType::None) {
                item["name"] = getNameString(pSentence);
            }
            item["src"] = pSentence->pre_processed_text;
            inputBlock += item.dump() + "\n";
            id2SentenceMap[pSentence->index] = pSentence;
        }
        break;

    case TransEngine::Sakura:
        for (const auto& pSentence : batchToTransThisRound) {
            if (pSentence->nameType != NameType::None) {
                inputBlock += getNameString(pSentence) + ":::::" + pSentence->pre_processed_text + "\n";
            }
            else {
                inputBlock += pSentence->pre_processed_text + "\n";
            }
        }
        inputBlock.pop_back(); // 移除最后一个换行符
        break;
    default:
        throw std::runtime_error("不支持的 TransEngine 用于构建输入");
    }
}

void parseContent(std::string& content, std::vector<Sentence*>& batchToTransThisRound, std::map<int, Sentence*>& id2SentenceMap, const std::string& modelName,
    TransEngine transEngine, bool& parseError, int& parsedCount, std::shared_ptr<IController> controller, std::atomic<int>& completedSentences) {
    if (content.find("</think>") != std::string::npos) {
        content = content.substr(content.find("</think>") + 8);
    }
    switch (transEngine) {
    case TransEngine::ForGalTsv:
    {
        size_t start = content.find("NAME\tDST\tID");
        if (start == std::string::npos) {
            parseError = true;
            break;
        }

        std::stringstream ss(content.substr(start));
        std::string line;
        std::getline(ss, line); // Skip header

        while (parsedCount < batchToTransThisRound.size() && std::getline(ss, line)) {
            if (line.empty() || line.find("```") != std::string::npos) continue;
            auto parts = splitString(line, '\t');
            if (parts.size() < 3) {
                parseError = true;
                continue;
            }
            try {
                int id = std::stoi(parts[2]);
                if (id2SentenceMap.count(id)) {
                    if (parts[1].empty() && !id2SentenceMap[id]->pre_processed_text.empty()) {
                        parseError = true;
                        continue;
                    }
                    id2SentenceMap[id]->pre_translated_text = parts[1];
                    id2SentenceMap[id]->translated_by = modelName;
                    id2SentenceMap[id]->complete = true;
                    completedSentences++;
                    controller->updateBar(); // ForGalTsv
                    parsedCount++;
                }
            }
            catch (...) {
                parseError = true;
                continue;
            }
        }

    }
    break;

    case TransEngine::ForNovelTsv:
    {
        size_t start = content.find("DST\tID"); // or DST\tID
        if (start == std::string::npos) {
            parseError = true;
            break;
        }

        std::stringstream ss(content.substr(start));
        std::string line;
        std::getline(ss, line); // Skip header

        while (parsedCount < batchToTransThisRound.size() && std::getline(ss, line)) {
            if (line.empty() || line.find("```") != std::string::npos) continue;
            auto parts = splitString(line, '\t');
            if (parts.size() < 2) {
                parseError = true;
                continue;
            }
            try {
                int id = std::stoi(parts[1]);
                if (id2SentenceMap.count(id)) {
                    if (parts[0].empty() && !id2SentenceMap[id]->pre_processed_text.empty()) {
                        parseError = true;
                        continue;
                    }
                    id2SentenceMap[id]->pre_translated_text = parts[0];
                    id2SentenceMap[id]->translated_by = modelName;
                    id2SentenceMap[id]->complete = true;
                    completedSentences++;
                    controller->updateBar(); // ForNovelTsv
                    parsedCount++;
                }
            }
            catch (...) {
                parseError = true;
                continue;
            }
        }
    }
    break;

    case TransEngine::ForGalJson:
    case TransEngine::DeepseekJson:
    {
        size_t start = std::min(content.find("{\"id\""), content.find("{\"dst\""));
        if (start == std::string::npos) {
            parseError = true;
            break;
        }

        std::stringstream ss(content.substr(start));
        std::string line;
        while (parsedCount < batchToTransThisRound.size() && std::getline(ss, line)) {
            if (line.empty() || !line.starts_with('{')) continue;
            try {
                json item = json::parse(line);
                int id = item.at("id");
                if (id2SentenceMap.count(id)) {
                    if (item.at("dst").empty() && !id2SentenceMap[id]->pre_processed_text.empty()) {
                        parseError = true;
                        continue;
                    }
                    id2SentenceMap[id]->pre_translated_text = item.at("dst");
                    id2SentenceMap[id]->translated_by = modelName;
                    id2SentenceMap[id]->complete = true;
                    completedSentences++;
                    controller->updateBar(); // ForGalJson/DeepseekJson
                    parsedCount++;
                }
            }
            catch (...) {
                parseError = true;
                continue;
            }
        }
    }
    break;

    case TransEngine::Sakura:
    {
        auto lines = splitString(content, '\n');
        // 核心检查：行数是否匹配
        if (lines.size() != batchToTransThisRound.size()) {
            parseError = true;
            break;
        }

        for (size_t i = 0; i < lines.size(); ++i) {
            std::string translatedLine = lines[i];
            Sentence* currentSentence = batchToTransThisRound[i];

            // 尝试剥离说话人
            if (!currentSentence->name.empty() && translatedLine.find(":::::") != std::string::npos) {
                size_t msgStart = translatedLine.find(":::::");
                translatedLine = translatedLine.substr(msgStart + 5);
            }

            currentSentence->pre_translated_text = translatedLine;
            currentSentence->translated_by = modelName;
            currentSentence->complete = true;
            completedSentences++;
            controller->updateBar(); // Sakura
            parsedCount++;
        }
    }
    break;
    default:
        throw std::runtime_error("不支持的 TransEngine 用于解析输出");
    }
}

void combineOutputFiles(const fs::path& originalRelFilePath, const std::map<fs::path, bool>& splitFileParts,
    std::shared_ptr<spdlog::logger> logger, const fs::path& outputCacheDir, const fs::path& outputDir) {

    ordered_json combinedJson = ordered_json::array();

    std::ifstream ifs;
    logger->debug("开始合并文件: {}", wide2Ascii(originalRelFilePath));

    std::vector<fs::path> partPaths;
    for (const auto& [relPartPath, ready] : splitFileParts) {
        partPaths.push_back(relPartPath);
    }

    std::ranges::sort(partPaths, [&](const fs::path& a, const fs::path& b)
        {
            size_t posA = a.filename().wstring().rfind(L"_part_");
            size_t posB = b.filename().wstring().rfind(L"_part_");
            if (posA == std::wstring::npos || posB == std::wstring::npos) {
                return false;
            }
            std::wstring numA = a.filename().wstring().substr(posA + 6, a.filename().wstring().length() - posA - 11);
            std::wstring numB = b.filename().wstring().substr(posB + 6, b.filename().wstring().length() - posB - 11);
            return std::stoi(numA) < std::stoi(numB);
        });

    for (const auto& relPartPath : partPaths) {
        fs::path partPath = outputCacheDir / relPartPath;
        if (fs::exists(partPath)) {
            try {
                ifs.open(partPath);
                ordered_json partData = ordered_json::parse(ifs);
                ifs.close();
                combinedJson.insert(combinedJson.end(), partData.begin(), partData.end());
            }
            catch (const json::exception& e) {
                ifs.close();
                logger->critical("合并文件 {} 时出错", wide2Ascii(partPath));
                throw std::runtime_error(e.what());
            }
        }
        else {
            throw std::runtime_error(std::format("试图合并 {} 时出错，缺少文件 {}", wide2Ascii(originalRelFilePath), wide2Ascii(partPath)));
        }
    }

    fs::path finalOutputPath = outputDir / originalRelFilePath;
    createParent(finalOutputPath);
    std::ofstream ofs(finalOutputPath);
    ofs << combinedJson.dump(2);
    logger->info("文件 {} 合并完成，已保存到 {}", wide2Ascii(originalRelFilePath), wide2Ascii(finalOutputPath));
}


bool hasRetranslKey(const std::vector<CheckSeCondFunc>& retranslKeys, const json& item, const Sentence* currentSe) {
    if (retranslKeys.empty()) {
        return false;
    }

    Sentence se;
    if (item.contains("name")) {
        se.nameType = NameType::Single;
        se.name = item.at("name");
        se.name_preview = item.at("name_preview");
    }
    else if (item.contains("names")) {
        se.nameType = NameType::Multiple;
        se.names = item.at("names");
        se.names_preview = item.at("names_preview");
    }
    se.original_text = item.value("original_text", "");
    se.pre_processed_text = item.value("pre_processed_text", "");
    se.pre_translated_text = item.value("pre_translated_text", "");
    if (item.contains("problems")) {
        se.problems = item.at("problems").get<std::vector<std::string>>();
    }
    if (item.contains("other_info")) {
        se.other_info = item.at("other_info").get<std::map<std::string, std::string>>();
    }
    se.translated_by = item.value("translated_by", "");
    se.translated_preview = item.value("translated_preview", "");
    se.prev = currentSe->prev;
    se.next = currentSe->next;

    return std::ranges::any_of(retranslKeys, [&](const CheckSeCondFunc& key)
        {
            return key(&se);
        });
}

void saveCache(const std::vector<Sentence>& allSentences, const fs::path& cachePath) {
    json cacheJson = json::array();
    for (const auto& se : allSentences) {
        if (!se.complete) {
            continue;
        }
        json cacheObj;
        cacheObj["index"] = se.index;
        if (se.nameType == NameType::Single) {
            cacheObj["name"] = se.name;
            cacheObj["name_preview"] = se.name_preview;
        }
        else if (se.nameType == NameType::Multiple) {
            cacheObj["names"] = se.names;
            cacheObj["names_preview"] = se.names_preview;
        }
        cacheObj["original_text"] = se.original_text;
        if (!se.other_info.empty()) {
            cacheObj["other_info"] = se.other_info;
        }
        cacheObj["pre_processed_text"] = se.pre_processed_text;
        cacheObj["pre_translated_text"] = se.pre_translated_text;
        if (!se.problems.empty()) {
            cacheObj["problems"] = se.problems;
        }
        cacheObj["translated_by"] = se.translated_by;
        cacheObj["translated_preview"] = se.translated_preview;
        cacheJson.push_back(cacheObj);
    }
    std::ofstream ofs(cachePath);
    ofs << cacheJson.dump(2);
}


std::vector<json> splitJsonArrayNum(const json& originalData, int chunkSize) {
    if (chunkSize <= 0 || !originalData.is_array() || originalData.empty()) {
        return { originalData };
    }

    std::vector<json> parts;
    size_t totalSize = originalData.size();

    for (size_t i = 0; i < totalSize; i += chunkSize) {
        size_t end = std::min(i + chunkSize, totalSize);
        json part = json::array();
        for (size_t j = i; j < end; ++j) {
            part.push_back(originalData[j]);
        }
        parts.push_back(part);
    }
    return parts;
}


std::vector<json> splitJsonArrayEqual(const json& originalData, int numParts) {
    if (numParts <= 1 || !originalData.is_array() || originalData.empty()) {
        return { originalData };
    }

    std::vector<json> parts;
    size_t totalSize = originalData.size();
    size_t partSize = totalSize / numParts;
    size_t remainder = totalSize % numParts;
    size_t currentIndex = 0;

    for (int i = 0; i < numParts; ++i) {
        size_t currentPartSize = partSize + (i < remainder ? 1 : 0);
        if (currentPartSize == 0) continue;

        json part = json::array();
        for (size_t j = 0; j < currentPartSize; ++j) {
            part.push_back(originalData[currentIndex + j]);
        }
        parts.push_back(part);
        currentIndex += currentPartSize;
    }
    return parts;
}
