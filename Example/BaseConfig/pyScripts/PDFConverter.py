# filename: babeldoc_api.py
import argparse
import json
import logging
import tempfile
from pathlib import Path
from threading import Lock
from concurrent.futures import Executor, Future
from typing import Tuple, List, Optional

# --- 核心BabelDOC库的导入 ---
from babeldoc.main import create_parser
from babeldoc.format.pdf.high_level import do_translate, TRANSLATE_STAGES
from babeldoc.format.pdf.translation_config import TranslationConfig, WatermarkOutputMode
from babeldoc.format.pdf.document_il.midend import il_translator
from babeldoc.translator.translator import BaseTranslator
from babeldoc.docvision.doclayout import DocLayoutModel
# 异常处理可能需要
from babeldoc.babeldoc_exception.BabelDOCException import ExtractTextError

# --- 修改后的 Import ---
import babeldoc.progress_monitor as progress_monitor

# --- Rich库用于创建漂亮的进度条 ---
from rich.progress import (
    BarColumn, MofNCompleteColumn, Progress, TextColumn, TimeElapsedColumn, TimeRemainingColumn
)

# --- 共享的辅助类 (与原版相同) ---

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
        if self.pm.pbar_manager:
            self.pbar = self.pm.pbar_manager.add_task(self.name, total=self.total)
        return self
    def __exit__(self, exc_type, exc_val, exc_tb):
        if hasattr(self, "pbar"):
            with self.lock:
                diff = self.total - self.current
                if diff > 0:
                    self.pm.pbar_manager.update(self.pbar, advance=diff)
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

# --- 内部核心函数 ---

def _extract_texts_from_pdf(pdf_path: Path, show_progress: bool) -> list[str]:
    """内部函数：从PDF中提取原文列表。"""
    text_collector = PdfTextCollector()
    with tempfile.TemporaryDirectory() as temp_dir:
        parser = create_parser()
        # 构造一个最小化的参数列表
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
            
            with progress_monitor.ProgressMonitor(TRANSLATE_STAGES) as pm:
                pbar = CustomTranslationStage.create_progress_bar() if show_progress else None
                pm.pbar_manager = pbar
                try:
                    if pbar: pbar.start()
                    do_translate(pm, config)
                except FinishReading:
                    pass # 预期的中断
                finally:
                    if pbar: pbar.stop()
        finally:
            # 恢复原始组件
            il_translator.PriorityThreadPoolExecutor = original_executor
            il_translator.ILTranslator.translate = original_il_translate
            progress_monitor.TranslationStage = original_stage
    return text_collector.source_texts

def _generate_pdf_from_texts(
    original_pdf: Path, translated_texts: list[str], output_dir: Path,
    no_mono: bool, no_dual: bool, show_progress: bool
) -> Tuple[Optional[str], Optional[str]]:
    """内部函数：使用译文列表生成PDF。"""
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
    result_paths = (None, None)
    try:
        il_translator.PriorityThreadPoolExecutor = MainThreadExecutor
        progress_monitor.TranslationStage = CustomTranslationStage
        with progress_monitor.ProgressMonitor(TRANSLATE_STAGES) as pm:
            pbar = CustomTranslationStage.create_progress_bar() if show_progress else None
            pm.pbar_manager = pbar
            try:
                if pbar: pbar.start()
                result = do_translate(pm, config)
                # 获取绝对路径
                mono_path = str(Path(result.mono_pdf_path).resolve()) if result.mono_pdf_path else None
                dual_path = str(Path(result.dual_pdf_path).resolve()) if result.dual_pdf_path else None
                result_paths = (mono_path, dual_path)
            finally:
                if pbar: pbar.stop()
    finally:
        il_translator.PriorityThreadPoolExecutor = original_executor
        progress_monitor.TranslationStage = original_stage
    return result_paths

# ==============================================================================
# ============================ C++ 可调用的 API 函数 ============================
# ==============================================================================

def api_extract(input_pdf_path: str, output_json_path: str, show_progress: bool = False) -> Tuple[bool, str]:
    """
    API函数：从PDF中提取文本并保存为JSON。
    
    :param input_pdf_path: 输入PDF文件的路径。
    :param output_json_path: 输出JSON文件的路径。
    :param show_progress: 是否在控制台显示进度条。
    :return: 一个元组 (success, message)，success为布尔值，message为描述信息。
    """
    try:
        pdf_path = Path(input_pdf_path)
        json_path = Path(output_json_path)
        
        if not pdf_path.exists():
            return False, f"Error: Input PDF not found at '{input_pdf_path}'"
        
        if show_progress: print(f"[*] Extracting original texts from `{pdf_path.name}`...")
        
        extracted_texts = _extract_texts_from_pdf(pdf_path, show_progress)
        
        if not extracted_texts:
            return True, "Warning: No text was extracted. JSON file will not be created."
        
        json_data = [{"message": text} for text in extracted_texts]
        
        if show_progress: print(f"[*] Writing {len(json_data)} text items to `{json_path.name}`...")
        
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(json_data, f, ensure_ascii=False, indent=2)
            
        return True, f"Success: Extracted {len(json_data)} items to '{str(json_path.resolve())}'"

    except (ExtractTextError, IOError, Exception) as e:
        logging.exception("An error occurred during text extraction:")
        return False, f"Fatal Error during extraction: {e}"

def api_apply(
    original_pdf_path: str,
    translation_json_path: str,
    output_dir_path: str,
    no_mono: bool = False,
    no_dual: bool = False,
    show_progress: bool = False
) -> Tuple[bool, str]:
    """
    API函数：将翻译JSON应用回PDF。

    :param original_pdf_path: 原始PDF文件路径。
    :param translation_json_path: 翻译JSON文件路径。
    :param output_dir_path: 输出PDF的目录。
    :param no_mono: 是否不生成纯译文PDF。
    :param no_dual: 是否不生成双语PDF。
    :param show_progress: 是否在控制台显示进度条。
    :return: 一个元组 (success, message)，success为布尔值，message为描述信息或成功时生成的文件路径。
    """
    try:
        pdf_path = Path(original_pdf_path)
        json_path = Path(translation_json_path)
        output_dir = Path(output_dir_path)

        if not pdf_path.exists():
            return False, f"Error: Original PDF not found at '{original_pdf_path}'"
        if not json_path.exists():
            return False, f"Error: Translation JSON not found at '{translation_json_path}'"
        
        output_dir.mkdir(parents=True, exist_ok=True)

        if show_progress: print(f"[*] Step 1/3: Extracting original texts for validation...")
        original_texts = _extract_texts_from_pdf(pdf_path, show_progress)
        if not original_texts:
            return False, "Error: Could not extract any text from the original PDF for validation."

        if show_progress: print(f"[*] Step 2/3: Loading and validating translation file...")
        try:
            with open(json_path, 'r', encoding='utf-8') as f:
                translated_data = json.load(f)
                translated_texts = [item['message'] for item in translated_data]
        except (IOError, json.JSONDecodeError, KeyError) as e:
            return False, f"Error: Failed to read or parse JSON file. Details: {e}"

        if len(original_texts) != len(translated_texts):
            msg = f"Fatal Error: Mismatch in text counts. Original: {len(original_texts)}, Translated: {len(translated_texts)}"
            return False, msg
        
        if show_progress: print(f"[+] Validation successful. Count: {len(translated_texts)}.")
        if show_progress: print(f"[*] Step 3/3: Generating translated PDF(s)...")

        mono_path, dual_path = _generate_pdf_from_texts(
            pdf_path, translated_texts, output_dir, no_mono, no_dual, show_progress
        )

        paths = []
        if mono_path: paths.append(f"Mono PDF: {mono_path}")
        if dual_path: paths.append(f"Dual PDF: {dual_path}")
        
        if not paths:
            return True, "Task completed, but no PDF was generated (check --no-mono/--no-dual flags)."
        
        return True, "\n".join(["Success! Generated files:"] + paths)

    except Exception as e:
        logging.exception("An error occurred during PDF generation:")
        return False, f"Fatal Error during PDF generation: {e}"

# ==============================================================================
# ============================ 命令行接口 (保持不变) ============================
# ==============================================================================

def main_cli():
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
        
        # 调用API函数，并显示进度
        success, message = api_extract(str(args.input_pdf), str(output_file), show_progress=True)

        if success:
            print(f"[✔] {message}")
        else:
            print(f"[!] {message}")

    elif args.command == 'apply':
        if not args.original_pdf.exists():
            print(f"[!] 错误: 原始PDF文件不存在 -> {args.original_pdf}")
            return
        if not args.translation_json.exists():
            print(f"[!] 错误: 翻译JSON文件不存在 -> {args.translation_json}")
            return

        # 调用API函数，并显示进度
        success, message = api_apply(
            str(args.original_pdf),
            str(args.translation_json),
            str(args.output_dir),
            args.no_mono,
            args.no_dual,
            show_progress=True
        )
        
        print("\n--- 任务报告 ---")
        if success:
            print(f"[✔] {message}")
        else:
            print(f"[!] {message}")
        print("-----------------")


if __name__ == "__main__":
    main_cli()
