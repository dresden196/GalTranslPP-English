# filename: PDFConverter.py
# -*- coding: utf-8 -*-

# ==============================================================================
# I. 环境修复与多进程禁用 (核心)
# ------------------------------------------------------------------------------
# 目的: 确保脚本在被C++等外部程序调用时，不会因多进程问题而出错。
# 措施:
#   1. set_executable: (仅Windows) 强制子进程使用正确的python.exe，避免
#      启动C++主程序从而引发"Unknown options"错误。
#   2. 猴子补丁 (Monkey Patching): 通过用单线程/空操作的实现替换掉
#      multiprocessing的核心组件(Process, Pool)，从根本上禁止创建新进程。
# 注意: 这部分代码必须在导入任何可能使用多进程的库(如babeldoc)之前执行。
# ==============================================================================
import sys
import os
import multiprocessing
import concurrent.futures
from concurrent.futures import Executor, Future
import logging

# --- Part 1: 纠正Windows环境下的子进程执行路径 ---
if sys.platform == "win32":
    # 在嵌入式环境中，sys.executable可能指向C++宿主程序。
    # 我们需要找到并指定真正的Python解释器路径。
    python_exe_path = os.path.join(sys.prefix, "python.exe")
    if not os.path.exists(python_exe_path):
        python_exe_path = os.path.join(sys.prefix, "Scripts", "python.exe")
    
    if os.path.exists(python_exe_path):
        multiprocessing.set_executable(python_exe_path)

# --- Part 2: 定义用于替换的伪造类 ---
class _NoOpProcess:
    """一个假的Process类，它的所有方法都是空操作，以阻止创建任何子进程。"""
    def __init__(self, *args, **kwargs): pass
    def start(self): pass
    def join(self, *args, **kwargs): pass
    def is_alive(self): return False
    @property
    def exitcode(self): return 0

class _MainThreadExecutor(Executor):
    """一个强制在当前线程同步执行的伪造执行器。"""
    def __init__(self, *args, **kwargs): pass
    def submit(self, fn, /, *args, **kwargs):
        if "priority" in kwargs: del kwargs["priority"]
        f = Future()
        try:
            f.set_result(fn(*args, **kwargs))
        except Exception as e:
            f.set_exception(e)
        return f
    def shutdown(self, wait: bool = True, *, cancel_futures: bool = False): pass
    def map(self, func, iterable, chunksize=None): return map(func, iterable)
    def apply_async(self, func, args=(), kwds={}, callback=None, error_callback=None):
        try:
            result = func(*args, **kwds)
            if callback: callback(result)
            future = Future(); future.set_result(result); return future
        except Exception as e:
            if error_callback: error_callback(e)
            future = Future(); future.set_exception(e); return future

# --- Part 3: 应用全局猴子补丁 ---
multiprocessing.Process = _NoOpProcess
multiprocessing.Pool = _MainThreadExecutor
concurrent.futures.ProcessPoolExecutor = _MainThreadExecutor

# ==============================================================================
# II. 标准库与第三方库导入
# ==============================================================================
import argparse
import json
import tempfile
from pathlib import Path
from threading import Lock
from typing import Tuple, List, Optional

# --- BabelDOC 核心库 ---
from babeldoc.main import create_parser
from babeldoc.format.pdf.high_level import do_translate, TRANSLATE_STAGES
from babeldoc.format.pdf.translation_config import TranslationConfig, WatermarkOutputMode
from babeldoc.format.pdf.document_il.midend import il_translator
from babeldoc.translator.translator import BaseTranslator
from babeldoc.docvision.doclayout import DocLayoutModel
from babeldoc.babeldoc_exception.BabelDOCException import ExtractTextError
import babeldoc.progress_monitor as progress_monitor

# --- Rich 进度条库 ---
from rich.progress import (
    BarColumn, MofNCompleteColumn, Progress, TextColumn, TimeElapsedColumn, TimeRemainingColumn
)

# --- 确保BabelDOC内部的执行器也被替换 ---
il_translator.PriorityThreadPoolExecutor = _MainThreadExecutor

# ==============================================================================
# III. 辅助工具类与函数
# ==============================================================================
class FinishReading(Exception):
    """自定义异常，用于在完成文本收集后安全地退出BabelDOC流程。"""
    @classmethod
    def raise_after_call(cls, func):
        def wrapper(*args, **kwargs):
            func(*args, **kwargs)
            raise FinishReading
        return wrapper

class PdfTextCollector(BaseTranslator):
    """伪装的翻译器，只收集文本而不进行翻译。"""
    def __init__(self):
        super().__init__('', '', True)
        self.source_texts: list[str] = []
    def translate(self, text, *args, **kwargs):
        self.source_texts.append(text)
        return text
    def do_translate(self, text, rate_limit_params: dict = None): return self.translate(text)
    def do_llm_translate(self, text, rate_limit_params: dict = None): raise NotImplementedError

class IgnoreFinishReadingExceptionFilter(logging.Filter):
    """日志过滤器，避免在控制台打印我们预期的FinishReading异常。"""
    def filter(self, record):
        if record.exc_info:
            return not issubclass(record.exc_info[0], FinishReading)
        return True

logging.getLogger("babeldoc.format.pdf.high_level").addFilter(IgnoreFinishReadingExceptionFilter())

class SequentialTranslator(BaseTranslator):
    """按顺序提供预设译文的翻译器。"""
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
            logging.warning(f"Translation text exhausted. Keeping original: '{text}'")
            return text
    def do_translate(self, text, rate_limit_params: dict = None): return self.translate(text)
    def do_llm_translate(self, text, rate_limit_params: dict = None): raise NotImplementedError

class CustomTranslationStage(progress_monitor.TranslationStage):
    """自定义的Rich进度条显示阶段。"""
    def __enter__(self):
        if self.pm.pbar_manager:
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

# ==============================================================================
# IV. 核心业务逻辑函数
# ==============================================================================
def _extract_texts_from_pdf(pdf_path: Path, show_progress: bool) -> list[str]:
    """内部函数：从PDF中提取原文列表。"""
    text_collector = PdfTextCollector()
    with tempfile.TemporaryDirectory() as temp_dir:
        parser = create_parser()
        args = parser.parse_args(["--files", str(pdf_path), "--output", temp_dir, "--no-mono", "--no-dual", "--ignore-cache"])
        config = TranslationConfig(
            input_file=args.files[0], output_dir=args.output, translator=text_collector,
            doc_layout_model=DocLayoutModel.load_onnx(), watermark_output_mode=WatermarkOutputMode.NoWatermark,
            skip_scanned_detection=True, lang_in=args.lang_in, lang_out=args.lang_out
        )
        original_il_translate = il_translator.ILTranslator.translate
        original_stage = progress_monitor.TranslationStage
        try:
            il_translator.ILTranslator.translate = FinishReading.raise_after_call(original_il_translate)
            progress_monitor.TranslationStage = CustomTranslationStage
            with progress_monitor.ProgressMonitor(TRANSLATE_STAGES) as pm:
                pbar = CustomTranslationStage.create_progress_bar() if show_progress else None
                pm.pbar_manager = pbar
                try:
                    if pbar: pbar.start()
                    do_translate(pm, config)
                except FinishReading:
                    pass  # 预期的正常中断
                finally:
                    if pbar: pbar.stop()
        finally:
            il_translator.ILTranslator.translate = original_il_translate
            progress_monitor.TranslationStage = original_stage
    return text_collector.source_texts

def _generate_pdf_from_texts(
    original_pdf: Path, translated_texts: list[str], output_dir: Path,
    no_mono: bool, no_dual: bool, show_progress: bool
) -> Tuple[Optional[str], Optional[str]]:
    """内部函数：使用译文列表生成翻译后的PDF。"""
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
    original_stage = progress_monitor.TranslationStage
    result_paths = (None, None)
    try:
        progress_monitor.TranslationStage = CustomTranslationStage
        with progress_monitor.ProgressMonitor(TRANSLATE_STAGES) as pm:
            pbar = CustomTranslationStage.create_progress_bar() if show_progress else None
            pm.pbar_manager = pbar
            try:
                if pbar: pbar.start()
                result = do_translate(pm, config)
                mono_path = str(Path(result.mono_pdf_path).resolve()) if result.mono_pdf_path else None
                dual_path = str(Path(result.dual_pdf_path).resolve()) if result.dual_pdf_path else None
                result_paths = (mono_path, dual_path)
            finally:
                if pbar: pbar.stop()
    finally:
        progress_monitor.TranslationStage = original_stage
    return result_paths

# ==============================================================================
# V. 公开API接口 (供C++调用)
# ==============================================================================
def api_extract(input_pdf_path: str, output_json_path: str, show_progress: bool = False) -> Tuple[bool, str]:
    """API函数：从PDF中提取文本并保存为JSON。"""
    try:
        pdf_path, json_path = Path(input_pdf_path), Path(output_json_path)
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

    except Exception as e:
        logging.exception("An error occurred during text extraction:")
        return False, f"Fatal Error during extraction: {e}"

def api_apply(
    original_pdf_path: str, translation_json_path: str, output_dir_path: str,
    no_mono: bool = False, no_dual: bool = False, show_progress: bool = False
) -> Tuple[bool, str]:
    """API函数：将翻译JSON应用回PDF。"""
    try:
        pdf_path, json_path, output_dir = Path(original_pdf_path), Path(translation_json_path), Path(output_dir_path)
        if not pdf_path.exists(): return False, f"Error: Original PDF not found at '{original_pdf_path}'"
        if not json_path.exists(): return False, f"Error: Translation JSON not found at '{translation_json_path}'"
        output_dir.mkdir(parents=True, exist_ok=True)

        if show_progress: print(f"[*] Step 1/3: Extracting original texts for validation...")
        original_texts = _extract_texts_from_pdf(pdf_path, show_progress)
        if not original_texts: return False, "Error: Could not extract any text from the original PDF for validation."

        if show_progress: print(f"[*] Step 2/3: Loading and validating translation file...")
        try:
            with open(json_path, 'r', encoding='utf-8') as f:
                translated_texts = [item['message'] for item in json.load(f)]
        except (IOError, json.JSONDecodeError, KeyError) as e:
            return False, f"Error: Failed to read or parse JSON file. Details: {e}"

        if len(original_texts) != len(translated_texts):
            msg = f"Fatal Error: Mismatch in text counts. Original: {len(original_texts)}, Translated: {len(translated_texts)}"
            return False, msg
        
        if show_progress: print(f"[+] Validation successful. Count: {len(translated_texts)}.")
        if show_progress: print(f"[*] Step 3/3: Generating translated PDF(s)...")

        mono_path, dual_path = _generate_pdf_from_texts(pdf_path, translated_texts, output_dir, no_mono, no_dual, show_progress)

        paths = [path for path in (mono_path, dual_path) if path]
        if not paths:
            return True, "Task completed, but no PDF was generated (check --no-mono/--no-dual flags)."
        
        report = ["Success! Generated files:"]
        if mono_path: report.append(f"Mono PDF: {mono_path}")
        if dual_path: report.append(f"Dual PDF: {dual_path}")
        return True, "\n".join(report)

    except Exception as e:
        logging.exception("An error occurred during PDF generation:")
        return False, f"Fatal Error during PDF generation: {e}"

# ==============================================================================
# VI. 命令行接口 (CLI)
# ==============================================================================
def main_cli():
    """处理命令行子命令和参数的主函数。"""
    parser = argparse.ArgumentParser(
        description="A tool for PDF text extraction and re-injection using BabelDOC.",
        formatter_class=argparse.RawTextHelpFormatter
    )
    subparsers = parser.add_subparsers(dest='command', required=True, help='Available sub-commands')

    # --- 'extract' command ---
    parser_extract = subparsers.add_parser('extract', help='Extract all text segments from a PDF into a JSON file.')
    parser_extract.add_argument("input_pdf", type=Path, help="Path to the input PDF file.")
    parser_extract.add_argument("output_json", type=Path, nargs='?', help="Path to the output JSON file.\nDefaults to the input filename with a .json extension.")

    # --- 'apply' command ---
    parser_apply = subparsers.add_parser('apply', help='Apply translated texts from a JSON file back to the original PDF.')
    parser_apply.add_argument("original_pdf", type=Path, help="Path to the original PDF file.")
    parser_apply.add_argument("translation_json", type=Path, help="Path to the JSON file containing translations.")
    parser_apply.add_argument("-o", "--output-dir", type=Path, default=Path.cwd(), help="Directory to save the output PDF(s).\nDefaults to the current working directory.")
    parser_apply.add_argument("--no-mono", action="store_true", help="Do not generate a translated-only (mono) PDF.")
    parser_apply.add_argument("--no-dual", action="store_true", help="Do not generate a bilingual (dual) PDF.")

    args = parser.parse_args()

    # --- Dispatch tasks based on command ---
    if args.command == 'extract':
        if not args.input_pdf.exists():
            print(f"[!] Error: Input PDF not found -> {args.input_pdf}")
            return
        output_file = args.output_json or args.input_pdf.with_suffix('.json')
        success, message = api_extract(str(args.input_pdf), str(output_file), show_progress=True)
        print(f"[✔] {message}" if success else f"[!] {message}")

    elif args.command == 'apply':
        if not args.original_pdf.exists():
            print(f"[!] Error: Original PDF not found -> {args.original_pdf}")
            return
        if not args.translation_json.exists():
            print(f"[!] Error: Translation JSON not found -> {args.translation_json}")
            return
        success, message = api_apply(
            str(args.original_pdf), str(args.translation_json), str(args.output_dir),
            args.no_mono, args.no_dual, show_progress=True
        )
        print("\n--- Task Report ---")
        print(f"[✔] {message}" if success else f"[!] {message}")
        print("-------------------")

if __name__ == "__main__":
    # 在直接作为脚本运行时，调用 multiprocessing.freeze_support() 是
    # Windows 平台下避免多进程递归错误的官方推荐做法。
    multiprocessing.freeze_support()
    main_cli()
