import stanza

class NLPProcessor:
    """
    一个纯粹的 Stanza NLP 处理器。
    """
    def __init__(self, model_name: str):
        """
        构造函数，加载指定的Stanza模型。
        :param model_name: Stanza的语言代码, e.g., 'en', 'zh'
        """
        self.nlp = stanza.Pipeline(lang=model_name, processors='tokenize,pos,ner', use_gpu=False, verbose=False)

    def process_text(self, text: str):
        """
        使用 Stanza 处理输入的文本块。
        """
        if not text:
            return [], []
        
        doc = self.nlp(text)
        
        word_pos_list = []
        for sentence in doc.sentences:
            for word in sentence.words:
                word_pos_list.append([word.text, word.upos])

        entity_list = []
        for sentence in doc.sentences:
            for ent in sentence.ents:
                entity_list.append([ent.text, ent.type])

        return word_pos_list, entity_list

def check_model(model_name: str) -> bool:
    """
    [修正版] 检查指定的 Stanza 模型（按语言）是否已完整下载。
    """
    try:
        stanza.Pipeline(lang=model_name, download_method=None, verbose=False)
        return True
    except Exception:
        return False

def get_download_command(model_name: str) -> list[str]:
    """
    生成用于下载指定 Stanza 模型的命令列表。
    """
    command_script = f"\"import stanza; stanza.download('{model_name}')\""
    return ["-c", command_script]
