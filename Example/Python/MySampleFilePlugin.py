# Required: Import C++ binding module
import gpp_plugin_api as gpp
from pathlib import Path
import tomllib
import threading
import queue
import time

# See GalTranslPP/PythonManager.cpp for all registered types and functions

# Required: Declare pythonTranslator
# C++ will assign base class pointer to this attribute before init
pythonTranslator = None

logger = None

def process_file(rel_file_path, worker_id):
    """Wrapper function, worker_id is the thread number"""
    try:
        pythonTranslator.m_controller.addThreadNum()
        pythonTranslator.processFile(rel_file_path, worker_id)
        pythonTranslator.m_controller.reduceThreadNum()
    except Exception as e:
        error_msg = f"[Thread {worker_id}] Error processing file {rel_file_path}: {str(e)}\n{traceback.format_exc()}"
        logger.error(error_msg)

def worker(worker_id, task_queue):
    """Worker thread function, worker_id is the fixed ID for this thread"""
    while True:
        task = task_queue.get()
        if task is None:  # Received exit signal
            task_queue.task_done()
            break

        try:
            rel_file_path = task  # Task only contains file path
            process_file(rel_file_path, worker_id)
        finally:
            task_queue.task_done()

def multi_threads_run(relFilePaths):
    # Multi-threading uses manual thread management
    # Because using multiprocessing or ThreadPoolExecutor in sub-interpreters has some quirky issues
    # Without LLM translation, behavior approaches single-threaded because I only release GIL on C++ side during LLM translation
    MAX_WORKERS = 29
    pythonTranslator.m_controller.makeBar(pythonTranslator.m_totalSentences, MAX_WORKERS)
    task_queue = queue.Queue()
    threads = []

    # Create and start worker threads, each with a fixed worker_id
    for worker_id in range(MAX_WORKERS):
        t = threading.Thread(
            target=worker,
            args=(worker_id, task_queue),  # Pass worker_id
            name=f"Worker-{worker_id}"
        )
        t.daemon = False
        t.start()
        threads.append(t)
        logger.info(f"Started worker thread {worker_id}")

    # Put all tasks into queue
    for relFilePath in relFilePaths:
        task_queue.put(relFilePath)
    logger.info(f"Added {len(relFilePaths)} tasks to queue")

    # Wait for all tasks to complete
    task_queue.join()

    # Send exit signal to all worker threads
    for _ in range(MAX_WORKERS):
        task_queue.put(None)

    # Wait for all threads to finish
    for t in threads:
        t.join(timeout=30)
        if t.is_alive():
            logger.warn(f"Thread {t.name} did not terminate properly")

    logger.info("All Python threads finished execution")

def run():
    # Each translator's filePlugin and textPlugin uses a dedicated sub-interpreter (except NLP tokenize series)
    # So this function can block without affecting other translator's work
    relFilePaths = pythonTranslator.normalJsonTranslator_beforeRun()
    if relFilePaths is None:
        logger.info("Probably a TransEngine like DumpName or GenDict that doesn't need processFile")
        return
    # Simple single-threaded processing
    # for relFilePath in relFilePaths:
    #     pythonTranslator.processFile(relFilePath, 0)
    multi_threads_run(relFilePaths)
    pythonTranslator.normalJsonTranslator_afterRun()
    return

    # Small example inheriting from Epub, also recommended to read Lua section first
    pythonTranslator.epubTranslator_beforeRun()
    # std::function<void(fs::path)>
    orgOnFileProcessedInEpubTranslator = pythonTranslator.m_onFileProcessed
    # Like Lua, this can also be a closure
    # But if you set this, then you must NOT use xxTranslator_process() series functions that create threads on C++ side
    # Otherwise it will cause deadlock because C++ side threads don't have GIL and can't call Python closures
    def new_call_back(rel_file_path_processed):
        logger.info("Intercepting to add a log entry meow")
        orgOnFileProcessedInEpubTranslator(rel_file_path_processed)
    pythonTranslator.m_onFileProcessed = new_call_back
    relFilePaths = pythonTranslator.normalJsonTranslator_beforeRun()
    if relFilePaths is None:
        logger.info("Probably a TransEngine like DumpName or GenDict that doesn't need processFile")
        return
    multi_threads_run(relFilePaths)
    pythonTranslator.normalJsonTranslator_afterRun()
    return

def init():
    # TOML config files and dictionary files are loaded after init
    pythonTranslator.normalJsonTranslator_init()
    # Recommended to read Lua plugin documentation first
    # pythonTranslator.epubTranslator_init()

    logger.info("MySampleFilePluginFromPython starts")
    logger.info("Current inputDir: " + str(pythonTranslator.m_inputDir))
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
        for splitFilePart, completed in filePartsMap.items():
            if completed:
                strs.append(str(splitFilePart))
            else:
                logger.error("This should not happen")
    result = "\n".join(strs)
    # logger.info("map keys: \n" + result)
