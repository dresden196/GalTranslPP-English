# check_spacy_gpu.py
import spacy

# spacy.prefer_gpu() 会尝试将默认设备设置为GPU
# 如果成功，它返回 True；如果失败（没有GPU或cupy），它返回 False
is_gpu_available = spacy.prefer_gpu()

if is_gpu_available:
    print("spaCy GPU support is available and enabled!")
    try:
        # require_gpu() 会在没有GPU时直接抛出错误
        spacy.require_gpu()
        print("spacy.require_gpu() confirmed GPU is active.")
    except Exception as e:
        print(f"Error during require_gpu(): {e}")
else:
    print("spaCy GPU support is NOT available. It will run on CPU.")

