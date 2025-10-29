# 必须: 导入 C++ 绑定的模块
import gpp_plugin_api as gpp
from pathlib import Path
import tomllib

# 必须: 声明 pythonTranslator
# C++ 会在 init 前将此属性赋值为基类指针
pythonTranslator = None

logger = None

def on_file_processed(rel_file_path: Path):
    logger.info(f"{rel_file_path} completed From Python")

def run():
    logger.info("暂时什么也不干")
    relFilePaths = pythonTranslator.normalJsonTranslator_beforeRun()
    pythonTranslator.m_onFileProcessed = on_file_processed
    for relFilePath in relFilePaths:
        pythonTranslator.processFile(relFilePath, 1)
    pythonTranslator.normalJsonTranslator_afterRun()

def init():
    logger.info("MySampleFilePluginFromPython starts")
    logger.info("apiStrategy: " + pythonTranslator.m_apiStrategy)
    tomlPath = pythonTranslator.m_projectDir / "config.toml"
    with open(tomlPath, "rb") as f:
        tomlConfig = tomllib.load(f)
        epubPreReg1 = tomlConfig['plugins']['Epub']['preprocRegex'][0]
        logger.info("{epubPreReg1} org: " + epubPreReg1['org'] + ", rep: " + epubPreReg1['rep'])

def unload():
    logger.info("MySampleFilePluginFromPython unloads")
    # std::map<fs::path, std::map<fs::path, bool>> m_jsonToSplitFileParts;
    partsTable = pythonTranslator.m_jsonToSplitFileParts
    strs = []
    for jsonPath, filePartsMap in partsTable.items():
        for splitFilePart, comp in filePartsMap.items():
            if comp:
                strs.append(str(splitFilePart))
            else:
                logger.error("不应该发生")
    result = "\n".join(strs)
    logger.info("map keys: \n" + result)