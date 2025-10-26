# 必须: 导入 C++ 绑定的模块
import gpp_plugin_api

# 全局变量，由 C++ 注入
logger = None
sourceLang_tokenizeFunc = None
targetLang_tokenizeFunc = None

# Sentence 的声明详见 Example/Lua/MySampleTextPlugin.lua

# --- 可选的分词器配置 ---
# 如果你的插件需要分词功能，取消注释并配置这些变量
# C++ 端会在初始化时读取它们
#
# sourceLang_useTokenizer = True
# sourceLang_tokenizerBackend = "spaCy"
# sourceLang_spaCyModelName = "ja_core_news_sm"
#
# targetLang_useTokenizer = True
# targetLang_tokenizerBackend = "pkuseg"
# ...

def checkConditionForRetranslKeysFunc(se: gpp_plugin_api.Sentence) -> bool:
    if se.index == 278:
        logger.info("RetransFromPython")
    return False

def checkConditionForSkipProblemsFunc(se: gpp_plugin_api.Sentence) -> bool:
    if se.index == 278:
        # retranslKey 中检查的 se 是一个副本，只负责检查，怎么设置都不会影响到要翻译和输出的部分
        # 但 skipProblems 的检查提供的是要输出的 se 的引用
        # 通过某个自定义的译后插件插入自定义的 flag problem
        # 再把 skipProblems 中最初的元素定义为此 problem 的全匹配检查
        # 甚至可达成在译后字典执行完毕后再执行一次插件的效果(区别是不会调用 init)
        current_problem = ""
        for i, s in enumerate(se.problems):
            # 正则检查通过后，执行 Condition 验证时，当前正在检查的 problem 会拥有前缀 'Current problem:'
            # PS: 不要在此检查函数中的任何地方以任何方式修改 problems 对象本身，可能导致 C++侧 出现异常
            # 但可以通过 problems_set_by_index 修改元素内容曲线救国
            if s.startswith("Current problem:"):
                current_problem = s[16:]
            else:
                success = se.problems_set_by_index(i, "My new problem")
                if success:
                    se.other_info |= { f"pySuccess{i}": "New problem suc" }
                else:
                    se.other_info |= { f"pyFail{i}": "New problem fail" }

        se.other_info |= { "pythonCheck": "The 278th" }
        se.other_info |= { "pycp": current_problem }
        logger.info("SkipProblemsFromPython")
    return False

def init(project_dir: str):
    """
    插件初始化函数，由 C++ 调用一次。
    """
    logger.info(f"Python plugin initialized for project: {project_dir}")】


def run(se: gpp_plugin_api.Sentence):
    """
    处理每个句子的主函数。
    se 是一个 C++ Sentence 对象的代理。
    """
    try:
        if se.next == None:
            # 和 Lua 不同，Python 里所有的 vector 和 map 属于复制过来的，不能直接 append
            se.problems += ["MySampleProblemForLastSentence"]
        if se.prev == None:
            se.other_info |= { "MyInfoKeyForFirstSentence": "MyTestValue" }

    except Exception as e:
        logger.error(f"Error in Python run function: {e}")
        raise

