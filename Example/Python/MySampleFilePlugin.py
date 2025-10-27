import gpp_plugin_api

#logger = None
#sourceLang_tokenizeFunc = None
#targetLang_tokenizeFunc = None

def on_file_processed(rel_file_path: str):
    logger.info(f"{rel_file_path} completed From Python")

def run():
    # 我不知道 C++ 调 py 的时候那个B GIL到底是怎么工作的，反正最后试出的结论就是只能开一个守护线程溜达着
    # 这就导致了在 python 脚本侧你不能执行任何阻塞操作 / 开辟新线程，Lua 就没有这个问题(因为它直接没有多线程...)
    # 所以 Lua 脚本的 normaljson_translator_run 是阻塞的，而 python 这边是强制半异步的
    # 在 C++ 下次调用此脚本的某个函数前 normaljson_translator_run 必定会执行完成，其余均不做保证
    normaljson_translator_run()

def init():
    # pybind11 的 function 转换有问题，不能像 Lua 那样直接传函数，所以就传当前文件的函数名好了...
    set_on_file_processed_str("on_file_processed")
    logger.info("MySampleFilePluginFromPython starts")
