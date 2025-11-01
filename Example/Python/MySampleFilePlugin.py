# 必须: 导入 C++ 绑定的模块
import gpp_plugin_api as gpp
from pathlib import Path
import tomllib
import threading
import queue
import time

# 必须: 声明 pythonTranslator
# C++ 会在 init 前将此属性赋值为基类指针
pythonTranslator = None

logger = None

def on_file_processed(rel_file_path: Path):
    logger.info(f"{rel_file_path} completed From Python")


def process_file(rel_file_path, index):
    pythonTranslator.processFile(rel_file_path, index)

def worker(task_queue):
    while True:
        try:
            rel_file_path, index = task_queue.get(timeout=1)
            if rel_file_path is None:  # 退出信号
                break
            process_file(rel_file_path, index)
        except queue.Empty:
            continue
        finally:
            task_queue.task_done()

def run():
    logger.info("准备 run run")
    relFilePaths = pythonTranslator.normalJsonTranslator_beforeRun()
    MAX_WORKERS = 29
    pythonTranslator.m_controller.makeBar(pythonTranslator.m_totalSentences, MAX_WORKERS)

    # 简单的单线程处理
    # for i, relFilePath in enumerate(relFilePaths):
    #     pythonTranslator.processFile(relFilePath, i)

    # 使用手动线程管理
    task_queue = queue.Queue()
    threads = []
    
    # 创建工作线程
    for _ in range(MAX_WORKERS):
        t = threading.Thread(target=worker, args=(task_queue,))
        t.daemon = True  # 设置为守护线程
        t.start()
        threads.append(t)
    
    # 添加任务
    for i, relFilePath in enumerate(relFilePaths):
        task_queue.put((relFilePath, i))
    
    # 等待所有任务完成
    task_queue.join()
    
    # 发送退出信号
    for _ in range(MAX_WORKERS):
        task_queue.put((None, None))
    
    # 等待所有线程结束
    for t in threads:
        t.join()

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