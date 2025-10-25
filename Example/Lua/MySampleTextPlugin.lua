--[[
enum class NameType
{
    None, Single, Multiple
};

struct Sentence {
    int index;
    std::string name;
    std::vector<std::string> names;
    std::string name_preview;
    std::vector<std::string> names_preview;
    std::string original_text;
    std::string pre_processed_text;
    std::string pre_translated_text;
    std::vector<std::string> problems;
    std::string translated_by;
    std::string translated_preview;
    std::string originalLinebreak;
    std::map<std::string, std::string> other_info;

    NameType nameType = NameType::None;
    Sentence* prev = nullptr;
    Sentence* next = nullptr;
    bool complete = false;
    bool notAnalyzeProblem = false;

    bool other_info_contains(const std::string& key);
    std::string other_info_get(const std::string& key);
    void other_info_set(const std::string& key, const std::string& value);
    std::tuple<std::vector<std::string>, std::vector<std::string>> other_info_get_all();
    void other_info_set_all(std::vector<std::string> keys, std::vector<std::string> values);
    void other_info_erase(const std::string& key);
    void other_info_clear();
    std::optional<std::string> problems_get_by_index(int index);
    bool problems_set_by_index(int index, const std::string& problem);
};

utils里的工具函数的签名详见 LuaMnager::registerCustomTypes 中绑定 utils 库的部分
]]

sourceLang_useTokenizer = false
targetLang_useTokenizer = true
--sourceLang_tokenizerBackend = "MeCab"
targetLang_tokenizerBackend = "pkuseg"
--sourceLang_meCabDictDir = "..."
--targetLang_spaCyModelName = "..."
--...

function checkConditionForRetranslKeysFunc(sentence)
    if sentence.index == 278 then 
        utils.logger:info("RetransFromLua")
    end
    return false
end

function checkConditionForSkipProblemsFunc(sentence)
    if sentence.index == 278 then 
        -- Example/Python/MySampleTextPlugin 中有对此函数的进一步介绍
        current_problem = ""
        for i, s in ipairs(sentence.problems) do
            if string.find(s, "^Current problem:") then 
                current_problem = string.sub(s, 17)
            else
                local success = sentence:problems_set_by_index(i-1, "My new problem")
                if success then
                    sentence:other_info_set("luaSuccess", "New problem suc")
                else
                    sentence:other_info_set("luaFail", "New problem fail")
                end
            end
        end
        sentence:other_info_set("luaCheck", "The 278th")
        sentence:other_info_set("luacp", current_problem) 
    end
    return false
end

function run(sentence)
    if sentence.index == 0 then
        sentence:other_info_set("MySampleKeyForFirstSentence", "MySampleValueForFirstSentence")
    end
    if sentence.next == nil and sentence.prev ~= nil and sentence.prev.nameType == NameType.None then
        sentence.problems:add("MySampleProblemForLastSentence")
        sentence.notAnalyzeProblem = true
    end
    if sentence.translated_preview == "久远紧锁眉头，脸上写着「骗人的吧，喂」。" then
        local wordpos_vec, entity_vec = utils.targetLang_tokenizeFunc(sentence.translated_preview)
        local tokens = utils.splitIntoTokens(wordpos_vec, sentence.translated_preview)
        local parts = {}
        for i, value in ipairs(wordpos_vec) do
            parts[i] = "[" .. value[1] .. "]"
        end
        local result = table.concat(parts, "#")
        sentence:other_info_set("tokens", result)
        utils.logger:info(string.format("测试 logger ，当前 index: %d", sentence.index))
    end
end

function init(projectDir)
    utils.logger:info(string.format("MySamplePlugin 初始化成功，projectDir: '%s'", projectDir))
end

function unload()
    local cmd = [[D:\GALGAME\linshi\天冥のコンキスタ\WORK\#toSjis.bat]]
    local cmd_acp = utils.ascii2Ascii(cmd, 65001, 0)
    local handle = io.popen(cmd_acp, 'r')
    if handle then
        local output = handle:read("*a")
        handle:close()
        utils.logger:info("--- BAT 脚本的输出内容 开始 ---\n" .. output .. "\n--- BAT 脚本的输出内容 结束 ---")
    else
        utils.logger:error("无法执行 BAT")
    end
end