import argparse
import json
import logging
import tempfile
from pathlib import Path
from threading import Lock
from concurrent.futures import Executor, Future

# --- 核心BabelDOC库的导入 ---
from babeldoc.main import create_parser
from babeldoc.format.pdf.high_level import do_translate, TRANSLATE_STAGES
from babeldoc.format.pdf.translation_config import TranslationConfig, WatermarkOutputMode
from babeldoc.format.pdf.document_il.midend import il_translator
from babeldoc.translator.translator import BaseTranslator
from babeldoc.docvision.doclayout import DocLayoutModel
from babeldoc.babeldoc_exception.BabelDOCException import ExtractTextError

# --- 修改后的 Import ---
import babeldoc.progress_monitor as progress_monitor

# --- Rich库用于创建漂亮的进度条 ---
from rich.progress import (
    BarColumn, MofNCompleteColumn, Progress, TextColumn, TimeElapsedColumn, TimeRemainingColumn
)

# --- 共享的辅助类 ---

class FinishReading(Exception):
    """自定义异常，用于在完成文本收集后安全地退出BabelDOC流程。"""
    @classmethod
    def raise_after_call(cls, func):
        def wrapper(*args, **kwargs):
            func(*args, **kwargs)
            raise FinishReading
        return wrapper

class PdfTextCollector(BaseTranslator):
    """伪装的翻译器，只收集文本，不翻译。"""
    def __init__(self):
        super().__init__('', '', True)
        self.source_texts: list[str] = []
    def translate(self, text, *args, **kwargs):
        self.source_texts.append(text)
        return text
    def do_translate(self, text, rate_limit_params: dict = None): return self.translate(text)
    def do_llm_translate(self, text, rate_limit_params: dict = None): raise NotImplementedError

class IgnoreFinishReadingExceptionFilter(logging.Filter):
    """日志过滤器，避免打印我们预期的FinishReading异常。"""
    def filter(self, record):
        if record.exc_info:
            return not issubclass(record.exc_info[0], FinishReading)
        return True

logging.getLogger("babeldoc.format.pdf.high_level").addFilter(IgnoreFinishReadingExceptionFilter())

class SequentialTranslator(BaseTranslator):
    """按顺序提供译文的翻译器，完美解决原文重复的问题。"""
    def __init__(self, translated_texts: list[str]):
        super().__init__('', '', True)
        self.translated_texts = translated_texts
        self.index = 0
        self.lock = Lock()
    def translate(self, text, *args, **kwargs):
        with self.lock:
            if self.index < len(self.translated_texts):
                result = self.translated_texts[self.index]
                self.index += 1
                return result
            else:
                logging.warning(f"译文数量不足，原文 '{text}' 将被保留。")
                return text
    def do_translate(self, text, rate_limit_params: dict = None): return self.translate(text)
    def do_llm_translate(self, text, rate_limit_params: dict = None): raise NotImplementedError

class MainThreadExecutor(Executor):
    """强制单线程执行器，确保顺序并简化逻辑。"""
    def __init__(self, *args, **kwargs) -> None: pass
    def submit(self, fn, /, *args, **kwargs):
        if "priority" in kwargs: del kwargs["priority"]
        f = Future()
        try:
            result = fn(*args, **kwargs)
            f.set_result(result)
        except Exception as e:
            f.set_exception(e)
        return f
    def shutdown(self, wait: bool = True, *, cancel_futures: bool = False) -> None: pass

class CustomTranslationStage(progress_monitor.TranslationStage):
    """自定义的进度条显示阶段。"""
    def __enter__(self):
        self.pbar = self.pm.pbar_manager.add_task(self.name, total=self.total)
        return self
    def __exit__(self, exc_type, exc_val, exc_tb):
        if hasattr(self, "pbar"):
            with self.lock:
                diff = self.total - self.current
                if diff > 0: self.pm.pbar_manager.update(self.pbar, advance=diff)
    def advance(self, n: int = 1):
        if hasattr(self, "pbar"):
            with self.lock:
                self.current += n
                self.pm.pbar_manager.update(self.pbar, advance=n)
    @staticmethod
    def create_progress_bar():
        return Progress(
            TextColumn("[progress.description]{task.description:<30}"), BarColumn(), MofNCompleteColumn(),
            TextColumn("•"), TimeElapsedColumn(), TextColumn("•"), TimeRemainingColumn()
        )

# --- 核心功能函数 ---

def extract_original_texts(pdf_path: Path) -> list[str]:
    """从PDF中提取原文列表，现在也带进度条。"""
    print(f"[*] 正在从PDF `{pdf_path.name}` 中提取原文...")
    text_collector = PdfTextCollector()
    with tempfile.TemporaryDirectory() as temp_dir:
        parser = create_parser()
        args = parser.parse_args([
            "--files", str(pdf_path), "--output", temp_dir, "--no-mono", "--no-dual", "--ignore-cache"
        ])
        config = TranslationConfig(
            input_file=args.files[0], output_dir=args.output, translator=text_collector,
            doc_layout_model=DocLayoutModel.load_onnx(), watermark_output_mode=WatermarkOutputMode.NoWatermark,
            skip_scanned_detection=True, lang_in=args.lang_in, lang_out=args.lang_out
        )
        
        original_executor = il_translator.PriorityThreadPoolExecutor
        original_il_translate = il_translator.ILTranslator.translate
        original_stage = progress_monitor.TranslationStage
        try:
            # 应用猴子补丁
            il_translator.PriorityThreadPoolExecutor = MainThreadExecutor
            il_translator.ILTranslator.translate = FinishReading.raise_after_call(original_il_translate)
            progress_monitor.TranslationStage = CustomTranslationStage
            
            with progress_monitor.ProgressMonitor(TRANSLATE_STAGES) as pm, CustomTranslationStage.create_progress_bar() as pbar:
                pm.pbar_manager = pbar
                try:
                    do_translate(pm, config)
                except FinishReading:
                    # 这是预期的中断
                    pass
        finally:
            # 恢复原始组件
            il_translator.PriorityThreadPoolExecutor = original_executor
            il_translator.ILTranslator.translate = original_il_translate
            progress_monitor.TranslationStage = original_stage

    print(f"\n[+] 成功提取 {len(text_collector.source_texts)} 条原文。")
    return text_collector.source_texts

def generate_translated_pdf(
    original_pdf: Path,
    translation_json: Path,
    output_dir: Path,
    no_mono: bool,
    no_dual: bool,
):
    """使用翻译JSON，生成新的PDF。"""
    # 步骤 1: 提取原文用于校验
    original_texts = extract_original_texts(original_pdf)
    if not original_texts:
        print("[!] 无法从PDF中提取原文，任务中止。")
        return

    # 步骤 2: 读取译文列表
    print(f"[*] 正在加载并校验翻译文件 `{translation_json.name}`...")
    try:
        with open(translation_json, 'r', encoding='utf-8') as f:
            translated_data = json.load(f)
            translated_texts = [item['message'] for item in translated_data]
    except (IOError, json.JSONDecodeError, KeyError) as e:
        print(f"[!] 错误: 无法读取或解析JSON文件。错误: {e}")
        return

    if len(original_texts) != len(translated_texts):
        print(f"[!] 严重错误: 原文数量 ({len(original_texts)}) 与译文数量 ({len(translated_texts)}) 不匹配！")
        return
    
    print(f"[+] 校验通过，原文与译文数量均为 {len(translated_texts)}。")
    
    # 步骤 3: 使用顺序翻译器生成PDF
    print(f"[*] 正在生成翻译后的PDF文件...")
    translator = SequentialTranslator(translated_texts)
    parser = create_parser()
    cmd_args = ["--files", str(original_pdf), "--output", str(output_dir), "--ignore-cache", "--skip-scanned-detection"]
    if no_mono: cmd_args.append("--no-mono")
    if no_dual: cmd_args.append("--no-dual")
    args = parser.parse_args(cmd_args)
    
    config = TranslationConfig(
        input_file=args.files[0], output_dir=args.output, translator=translator,
        doc_layout_model=DocLayoutModel.load_onnx(), no_dual=args.no_dual, no_mono=args.no_mono,
        watermark_output_mode=WatermarkOutputMode.NoWatermark,
        lang_in=args.lang_in, lang_out=args.lang_out
    )
    
    original_executor = il_translator.PriorityThreadPoolExecutor
    original_stage = progress_monitor.TranslationStage
    try:
        il_translator.PriorityThreadPoolExecutor = MainThreadExecutor
        progress_monitor.TranslationStage = CustomTranslationStage
        with progress_monitor.ProgressMonitor(TRANSLATE_STAGES) as pm, CustomTranslationStage.create_progress_bar() as pbar:
            pm.pbar_manager = pbar
            result = do_translate(pm, config)
        
        print("\n[✔] 任务完成！")
        if result.mono_pdf_path: print(f"  - 纯译文PDF: {Path(result.mono_pdf_path).resolve()}")
        if result.dual_pdf_path: print(f"  - 双语PDF: {Path(result.dual_pdf_path).resolve()}")
    except Exception as e:
        print(f"[!] 生成PDF时发生错误: {e}")
        logging.exception("详细错误信息:")
    finally:
        il_translator.PriorityThreadPoolExecutor = original_executor
        progress_monitor.TranslationStage = original_stage

def main():
    """主函数，处理命令行子命令和参数。"""
    parser = argparse.ArgumentParser(
        description="一个使用BabelDOC进行PDF文本提取与翻译回注的工具。",
        formatter_class=argparse.RawTextHelpFormatter
    )
    subparsers = parser.add_subparsers(dest='command', required=True, help='可用的子命令')

    # --- 定义 'extract' 子命令 ---
    parser_extract = subparsers.add_parser('extract', help='从PDF中提取所有文本段落并保存为JSON文件。')
    parser_extract.add_argument("input_pdf", type=Path, help="输入的PDF文件路径。")
    parser_extract.add_argument(
        "output_json", type=Path, nargs='?',
        help="输出的JSON文件路径。\n如果未提供，则默认为输入文件名加上.json后缀。"
    )

    # --- 定义 'apply' 子命令 ---
    parser_apply = subparsers.add_parser('apply', help='将JSON文件中的翻译文本回注到原始PDF中，生成新PDF。')
    parser_apply.add_argument("original_pdf", type=Path, help="原始的PDF文件路径。")
    parser_apply.add_argument("translation_json", type=Path, help="仅包含译文的JSON文件路径。")
    parser_apply.add_argument(
        "-o", "--output-dir", type=Path, default=Path.cwd(),
        help="输出PDF文件的目录路径。\n默认为当前工作目录。"
    )
    parser_apply.add_argument("--no-mono", action="store_true", help="不生成纯译文版本的PDF。")
    parser_apply.add_argument("--no-dual", action="store_true", help="不生成双语对照版本的PDF。")

    args = parser.parse_args()

    # --- 根据子命令分发任务 ---
    if args.command == 'extract':
        if not args.input_pdf.exists():
            print(f"[!] 错误: 输入PDF文件不存在 -> {args.input_pdf}")
            return
        
        output_file = args.output_json if args.output_json else args.input_pdf.with_suffix('.json')
        
        extracted_texts = extract_original_texts(args.input_pdf)

        if not extracted_texts:
            print("[*] 未提取到任何文本，不生成JSON文件。")
            return

        json_data = [{"message": text} for text in extracted_texts]
        
        print(f"[*] 正在将 {len(json_data)} 条文本写入到 `{output_file}`...")
        try:
            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(json_data, f, ensure_ascii=False, indent=2)
            print(f"[✔] 成功！JSON文件已保存到: {output_file.resolve()}")
        except IOError as e:
            print(f"[!] 错误: 无法写入文件 -> {e}")

    elif args.command == 'apply':
        if not args.original_pdf.exists():
            print(f"[!] 错误: 原始PDF文件不存在 -> {args.original_pdf}")
            return
        if not args.translation_json.exists():
            print(f"[!] 错误: 翻译JSON文件不存在 -> {args.translation_json}")
            return

        args.output_dir.mkdir(parents=True, exist_ok=True)
        generate_translated_pdf(
            args.original_pdf, args.translation_json, args.output_dir, args.no_mono, args.no_dual
        )

if __name__ == "__main__":
    main()
