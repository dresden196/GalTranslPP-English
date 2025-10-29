function on_file_processed(rel_file_path)
    -- 相对于原始读入文件夹的路径
    utils.logger:info(string.format("%s completed From Lua", rel_file_path.value))
end

function run()
    -- 这是 NormalJsonTranslator 要读取的原始 json 文件夹
    luaTranslator.m_inputDir = luaTranslator.m_projectDir / Path.new("gt_input")
    -- 这是 NormalJsonTranslator 翻译完成后的输出文件夹
    -- 重载了 Path 类型的 / 操作符，但要求其中至少一方是 Path 类型
    -- 用字符串对 Path 赋值可采用 varName = Path.new("str") 或 varName.value = "str"
    luaTranslator.m_outputDir = luaTranslator.m_projectDir / "gt_output"
    utils.logger:info(string.format("Current inputDir: %s", luaTranslator.m_inputDir))
    -- 有文件处理完毕后会回调这个函数
    luaTranslator.m_onFileProcessed = on_file_processed
    luaTranslator:normalJsonTranslator_run()

    -- -- Lua 没有多线程，所有函数都是阻塞的
    -- -- 但并不影响 TextPlugin，即使你的 conditionFunc 放在这个脚本里也没问题
    -- -- 因为 FilePlugin 和 TextPlugin 用的是不同的 LuaState(Lua环境)
    -- -- 如果想在 Lua 中使用 C++ NormalJsonTranslaor 的文件多线程
    -- -- 除过直接调用 run 函数之外
    -- -- 唯一的办法是使用 m_threadPool:push(luaTranslator , Vec{Paths}) (包括这个函数本身也是阻塞的)
    -- local relFilePaths = luaTranslator:normalJsonTranslator_beforeRun()
    -- if relFilePaths == nil then 
    --     utils.logger:info("可能是 DumpName 或 GenDict 之类无需 processFile 的 TransEngine")
    --     return
    -- end
    -- local relFilePathsStr = {}
    -- for i = 1, #relFilePaths do
    --     table.insert(relFilePathsStr, relFilePaths[i].value)
    -- end
    -- utils.logger:info("relFilePaths: \n" .. table.concat(relFilePathsStr, "\n"))
    -- luaTranslator.m_threadPool:resize(35)
    -- luaTranslator.m_controller:makeBar(luaTranslator.m_totalSentences, 35)
    -- luaTranslator.m_threadPool:push(luaTranslator, relFilePaths)
    -- luaTranslator:normalJsonTranslator_afterRun()

    -- luaTranslator:epubTranslator_beforeRun()
    -- -- std::function<void(fs::path)>
    -- orgOnFileProcessedInEpubTranslator = luaTranslator.m_onFileProcessed
    -- luaTranslator.m_onFileProcessed = function (relFilePathProcessed)
    --     utils.logger:info("拦截然后加一条日志喵")
    --     orgOnFileProcessedInEpubTranslator(relFilePathProcessed)
    -- end
    -- luaTranslator:normalJsonTranslator_run()
    
end

function init()
    utils.logger:info("MySampleFilePluginFromLua starts")
    utils.logger:info("apiStrategy: " .. luaTranslator.m_apiStrategy)
    local tomlConfig = toml.parse(luaTranslator.m_projectDir / "config.toml")
    if tomlConfig == nil then
        utils.logger:info("出错错了喵")
    else
        local epubPreReg1 = tomlConfig.plugins.Epub.preprocRegex[1]
        utils.logger:info("{epubPreReg1} org: " .. epubPreReg1.org .. ", rep: " .. epubPreReg1.rep)
    end
end

function countMap(map)
    local count = 0
    for k, v in pairs(map) do
        count = count + 1
    end
    return count
end

function unload()
    utils.logger:info("MySampleFilePluginFromLua unloads")
    -- std::map<fs::path, std::map<fs::path, bool>> m_jsonToSplitFileParts;
    local partsTable = luaTranslator.m_jsonToSplitFileParts
    local strs = {}
    for jsonPath, filePartsMap in pairs(partsTable) do
        for splitFilePart, comp in pairs(filePartsMap) do
            if comp then
                table.insert(strs, splitFilePart.value)
                local j = json.parse(luaTranslator.m_cacheDir / splitFilePart)
                if j == nil then
                    utils.logger:info("出错错了喵")
                elseif #j >= 1 then
                    table.insert(strs, j[1].translated_preview)
                end
            else
                utils.logger:error("不应该发生")
            end
        end
    end
    utils.logger:info("map keys: \n" .. table.concat(strs, "\n"))

    -- -- 为了能使用 pairs 遍历，map返回类型都是副本，如果想修改，必须 luaTranslator.mapVar = copy
    -- utils.logger:info("原有 " .. tostring(countMap(luaTranslator.m_jsonToSplitFileParts)) .. " 个 key-value 对")
    -- local newPartTable = { MyNewPartPath1 = false, ["MyNewPartPath2"] = false }
    -- luaTranslator.m_jsonToSplitFileParts[Path.new("MyNewJsonPathKey")] = newPartTable
    -- utils.logger:info("依然为 " .. tostring(countMap(luaTranslator.m_jsonToSplitFileParts)) .. " 个 key-value 对")
    -- partsTable[Path.new("MyNewJsonPathKey")] = newPartTable
    -- luaTranslator.m_jsonToSplitFileParts = partsTable
    -- utils.logger:info("现有 " .. tostring(countMap(luaTranslator.m_jsonToSplitFileParts)) .. " 个 key-value 对")

    -- local jsonToInfoMap = luaTranslator.m_jsonToInfoMap
    -- for jsonPath, jsonInfo in pairs(jsonToInfoMap) do
    --     utils.logger:info("metadataVecSize: " .. tostring(#jsonInfo.metadata))
    --     for i=1, #jsonInfo.metadata do
    --         utils.logger:info("offset: " .. tostring(jsonInfo.metadata[i].offset) .. ", length: " .. tostring(jsonInfo.metadata[i].length))
    --     end
    -- end

    utils.logger:info("MySampleFilePluginFromLua unloads")
end