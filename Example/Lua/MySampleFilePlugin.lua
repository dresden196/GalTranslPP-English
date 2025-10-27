function on_file_processed(rel_file_path)
    -- 相对于原始读入文件夹的路径
    utils.logger:info(string.format("%s completed From Lua", rel_file_path))
end

function run()
    luaTranslator:normalJsonTranslator_run()
end

function init()
    -- 这是 NormalJsonTranslator 要读取的原始 json 文件夹
    luaTranslator.m_inputDir = luaTranslator.m_projectDir .. "/gt_input"
    -- 这是 NormalJsonTranslator 翻译完成后的输出文件夹
    luaTranslator.m_outputDir = luaTranslator.m_projectDir .. "/gt_output"
    -- 有文件处理完毕后会回调这个函数
    luaTranslator.m_onFileProcessed = on_file_processed
    utils.logger:info(string.format("Current inputDir: %s", luaTranslator.m_inputDir))
    utils.logger:info("MySampleFilePluginFromLua starts")
end

function unload()
    utils.logger:info("MySampleFilePluginFromLua unloads")
end