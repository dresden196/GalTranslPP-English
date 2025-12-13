# Required: Import C++ binding module
import gpp_plugin_api as gpp
from pathlib import Path

# See Example/Lua/MySampleTextPlugin.lua for Sentence declaration details

# Global variables, injected by C++
# In Python, logger and tokenizeFunc are in the current module's globals, not in utils
logger = None

# sourceLang_useTokenizer = True
# sourceLang_tokenizerBackend = "MeCab"
# sourceLang_meCabDictDir = "..."
# sourceLang_spaCyModelName = "ja_core_news_sm"
# targetLang_spaCyModelName = "..."
# targetLang_stanzaLang = "..."
# sourceLang_tokenizeFunc = None
# ...

targetLang_useTokenizer = True
targetLang_tokenizerBackend = "pkuseg"
# If using provided tokenizer, must define tokenizeFunc first
targetLang_tokenizeFunc = None


def checkConditionForRetranslKeysFunc(se: gpp.Sentence) -> bool:
    if se.index == 278:
        logger.info("RetransFromPython")
    return False

def checkConditionForSkipProblemsFunc(se: gpp.Sentence) -> bool:
    if se.index == 278:
        # The se in retranslKey check is a copy, only for checking - modifications won't affect translation/output
        # But skipProblems check provides a reference to the actual se to be output
        # By inserting a custom flag problem via a custom post-processing plugin
        # and defining the first element in skipProblems as a full match check for this problem
        # You can even achieve running a plugin again after post-translation dict finishes
        # (difference: won't call init and unload)
        current_problem = ""
        for i, s in enumerate(se.problems):
            # After regex check passes, during Condition verification, the current problem has prefix 'Current problem:'
            # PS: DO NOT modify the problems object itself in any way within this check function - may cause C++ side exceptions
            # But you can modify element content indirectly via problems_set_by_index
            if s.startswith("Current problem:"):
                current_problem = s[16:]
            else:
                success = se.problems_set_by_index(i, "My new problem")
                if success:
                    se.other_info |= { f"pySuccess{i}": "New problem suc" }
                else:
                    se.other_info |= { f"pyFail{i}": "New problem fail" }

        se.other_info |= { "pythonCheck": "The 278th" }
        se.other_info |= { "pyCp": current_problem }
        logger.info("SkipProblemsFromPython")
    return False

def init(project_dir: Path):
    """
    Plugin initialization function, called once by C++.
    """
    logger.info(f"MySampleTextPluginFromLua initialized successfully, projectDir: {project_dir}")


def run(se: gpp.Sentence):
    """
    Main function to process each sentence.
    se is a proxy for a C++ Sentence object.
    """
    if se.translated_preview == "久远紧锁眉头，脸上写着「骗人的吧，喂」。":
        wordpos_vec, entity_vec = targetLang_tokenizeFunc(se.translated_preview)
        # Other utility functions (though not many) are still in utils
        tokens = gpp.utils.splitIntoTokens(wordpos_vec, se.translated_preview)
        parts = [f"[{value[0]}]" for value in wordpos_vec]
        result = "#".join(parts)
        # All containers are copies, direct insert/append etc. operations are ineffective
        se.other_info |= { "tokens_python": result }

def unload():
    pass
