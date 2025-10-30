# 必须: 导入 C++ 绑定的模块
import gpp_plugin_api as gpp
from pathlib import Path
import tomllib
import concurrent.futures
import time

# 必须: 声明 pythonTranslator
# C++ 会在 init 前将此属性赋值为基类指针
pythonTranslator = None

logger = None

def on_file_processed(rel_file_path: Path):
    logger.info(f"{rel_file_path} completed From Python")

def process_file(rel_file_path, thread_id):
    logger.info(f"Python 线程 {thread_id} 开始运行: {rel_file_path}")
    gpp.utils.registerPythonThread()
    logger.info(f"Python 线程 {thread_id} 注册线程完毕")
    pythonTranslator.processFile(rel_file_path, thread_id)
    #time.sleep(10)
    logger.info(f"Python 线程 {thread_id} C++ 函数运行完毕")
    gpp.utils.erasePythonThread()
    logger.info(f"Python 线程 {thread_id} 运行完毕")

def run():
    logger.info("准备 run run")
    relFilePaths = pythonTranslator.normalJsonTranslator_beforeRun()
    #pythonTranslator.m_onFileProcessed = on_file_processed
    MAX_WORKERS = 29
    pythonTranslator.m_controller.makeBar(pythonTranslator.m_totalSentences, MAX_WORKERS)
    # for i, relFilePath in enumerate(relFilePaths):
    #     pythonTranslator.processFile(relFilePath, i)
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        indices = range(len(relFilePaths))
        results = executor.map(process_file, relFilePaths, indices)
    logger.info("所有 Python 线程执行完毕")
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
    # logger.info("map keys: \n" + result)