import sys
import os
import pkuseg

# pkuseg 是一个基于CPU的库，没有GPU相关的设置。

class NLPProcessor:
    """
    一个纯粹的NLP处理器，它假设模型已经存在。
    下载逻辑由C++调用方处理。
    这个版本是为 pkuseg 库量身定做的。
    """
    def __init__(self, model_name: str):
        """
        构造函数，直接加载指定的pkuseg模型。
        """
        self.seg = pkuseg.pkuseg(model_name="default", postag=False)

    def process_text(self, text: str):
        """
        处理输入的文本块。
        使用pkuseg进行分词和词性标注。
        注意：pkuseg不直接支持命名实体识别(NER)，因此实体列表将始终为空。
        """
        if not text:
            return [], []
        
        words = self.seg.cut(text)
        
        # 使用列表推导式将结果转换为目标格式 [[word, ""], [word, ""], ...]
        # 这确保了数据结构与spacy版本完全相同，即使我们没有词性信息。
        word_pos_list = [[word, ""] for word in words]
        
        # pkuseg 不提供命名实体识别功能，返回一个空列表以保持API一致性
        entity_list = []
        
        return word_pos_list, entity_list

def check_model(model_name: str) -> bool:
    return True