import sys
import spacy

class NLPProcessor:
    """
    一个纯粹的NLP处理器，它假设模型已经存在。
    下载逻辑由C++调用方处理。
    """
    def __init__(self, model_name: str):
        """
        构造函数，直接加载指定的spaCy模型。
        如果模型不存在，spacy.load会抛出OSError，
        这个异常会被C++捕获。
        :param model_name: spaCy模型的名称, e.g., "en_core_web_sm"
        """
        self.nlp = spacy.load(model_name)

    def process_text(self, text: str):
        """
        处理输入的文本块。
        """
        if not text:
            return [], []
        doc = self.nlp(text)
        word_pos_list = [[token.text, token.pos_] for token in doc]
        entity_list = [[ent.text, ent.label_] for ent in doc.ents]
        return word_pos_list, entity_list

def check_model(model_name: str) -> bool:
    """
    检查指定的spaCy模型是否已安装。
    :param model_name: 模型名称
    :return: True 如果已安装, 否则 False
    """
    return spacy.util.is_package(model_name)

def get_download_command(model_name: str) -> list[str]:
    """
    生成用于下载指定spaCy模型的命令列表。
    使用 sys.executable 确保我们指向的是正确的Python解释器。
    :param model_name: 模型名称
    :return: 一个字符串列表，代表要执行的命令及其参数。
    """
    return ["-m", "spacy", "download", model_name]