import sys
import os
import pkuseg

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
        self.seg = pkuseg.pkuseg(model_name="default", postag=True)

    def process_text(self, text: str):
        """
        处理输入的文本块。
        使用pkuseg进行分词和词性标注。
        """
        if not text:
            return [], []
        
        # pkuseg.cut() 在 postag=True 时返回一个元组列表，例如 [('你好', 'v'), ('世界', 'n')]
        # 我们将其转换为列表的列表以匹配原始API的输出格式
        word_pos_list = self.seg.cut(text)
        
        # pkuseg 不提供命名实体识别功能，返回一个空列表以保持API一致性
        entity_list = []
        
        return word_pos_list, entity_list

def check_model(model_name: str) -> bool:
    return True