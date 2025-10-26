function on_file_processed(rel_file_path)
    -- 相对于原始读入文件夹的路径
    utils.logger:info(string.format("%s completed From Lua", rel_file_path))
end

function run()
    normaljson_translator_run()
end

function init()
    -- 这是 NormalJsonTranslator 要读取的原始 json 文件夹
    set_input_dir(get_project_dir() .. "/gt_input")
    -- 这是 NormalJsonTranslator 翻译完成后的输出文件夹
    set_output_dir(get_project_dir() .. "/gt_output")
    -- 有文件处理完毕后会回调这个函数
    set_on_file_processed(on_file_processed)
    utils.logger:info(string.format("Current inputDir: %s", get_input_dir()))
    utils.logger:info("MySampleFilePluginFromLua starts")
end

function unload()
    utils.logger:info("MySampleFilePluginFromLua unloads")
end