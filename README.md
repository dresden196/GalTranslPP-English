# GalTransl++

![GalTransl++ GUI](img/GalTranslPP.png?raw=true)
![GalTransl++ GUI En](img/GalTranslPP_en.png?raw=true)

**GalTransl++** inherits the core philosophy and architecture of [GalTransl](https://github.com/GalTransl/GalTransl) - a "project-centric" approach. It refines the best elements accumulated over two years of development while incorporating extensive feedback from visual novel patch creators, resulting in a lightweight, transparent translation engine with highly flexible and convenient extensibility.

**GalTransl++ GUI** wraps the GalTransl++ core with a Fluent UI-style interface, aiming to maintain high customizability even within a graphical environment. This is the primary focus of the project's development.

## ✨ Features

Building upon GalTransl's foundational functionality, this project includes optimizations for the following (and more):

* Improved single-file split caching
* Punctuation-priority line break fixing
* Optional regex-based (with ICU extensions) highly customizable pre/post-translation dictionaries with clear priority levels
* Highly customizable EPUB extraction
* Effective API quota exhaustion detection
* Pop-up card notifications on completion (GUI only)
* Better unused dictionary entry detection
* Customizable symbol detection
* Separate generation of preprocessing results for inspection
* Consolidated translation issue overview
* Faster rebuild process
* More convenient prompt customization
* Clearer dictionary usage settings
* Known issues attached when retranslating
* Customizable comparison targets for issue detection
* Custom Lua/Python scripting support for conditional logic, text processing, and file format handling

![notification](img/notification.png?raw=true)

## 📖 Workflow Guide

### GalTransl++ Translation Process and Dictionary Types

For those familiar with GalTransl, transitioning to the GalTransl++ CLI version is straightforward. The following focuses mainly on the GUI.

Let's briefly discuss the translation workflow (assuming you've read GalTransl's documentation - API setup and similar basics are omitted).

Regardless of file format, GalTransl++ ultimately normalizes everything into JSON for processing.

Each JSON file is a list of objects, where each object represents a "sentence" - the basic unit fed to the AI for translation.

When reading JSON, two keys are primarily relevant: `name` and `message`. Different translation modes (`transEngine`) process these two keys in various ways:


#### Translation Modes (transEngine)

* **`# ForGalJson`**: Production translation mode. Feeds JSON-formatted sentences (including `name` and `message`) to the AI and requires JSON-formatted responses. The program parses the returned JSON.
* **`# ForGalTsv`**: Production translation mode. Feeds TSV-formatted sentences (including `name` and `message`) to the AI and requires TSV-formatted responses. The program parses the returned TSV. May use fewer tokens than ForGalJson.
* **`# ForNovelTsv`**: Production translation mode. Similar to `ForGalTsv` but with modified prompts - the `name` key is omitted during input and parsing.
* **`# Sakura`**: Production translation mode. Feeds natural language-formatted sentences (including `name` and `message`) to the AI. Since Sakura is a translation-specialized model, it returns similarly formatted sentences without explicit requirements. The program parses the natural language response.
* **`# DumpName`**: Extracts all `name` keys and generates a `NameTable.toml` file in the project folder for unified name replacement (updates existing file if present).
* **`# GenDict`**: Uses AI to automatically generate a terminology glossary, saved to `Project GptDict-Generated.toml` in the project folder.
* **`# Rebuild`**: Skips retranslation even when `retranslKey` matches - only rebuilds results from cache.
* **`# ShowNormal`**: Saves preprocessed content and sentences to a folder with `show_normal` in its name within the project directory. For EPUB format, generates preprocessed HTML/XHTML files and the resulting JSON for inspection and debugging.

### Caching Mechanism

The cache mentioned in `Rebuild` refers to JSON files stored in the `trans_cache` folder within the project directory after translation. These contain the sequence number, original text, translated text, and other information for each file in order. All production translation modes first read from cache, then only translate sentences not yet cached.

> **⚠️ Important Note**: When using single-file splitting, cache hits incorporate context. Therefore, changing the file itself, split count, or split method may cause some unrelated sentences to miss the cache. Theoretically, the more files are split and the more the final split count exceeds the maximum thread count, the more sentences will miss the cache. GalTransl++ tries to maximize original cache hits in these situations, but for best cache performance, avoid changing split methods or counts. You can use `ShowNormal` mode to observe the split results.

`retranslKey` refers to retranslation keywords.

GalTransl++ automatically analyzes common translation issues during translation and outputs problems to the cache.

Normally, if `problems` contains a configured `retranslKey` (or other conditions), even if cache hits in production translation mode, that sentence will still be retranslated. So if you only want to rebuild from cache, either remove all `retranslKey` entries or use `Rebuild` mode to skip translation.

### Dictionary System

GalTransl++ dictionaries are divided into three categories: **Pre-Translation Dictionary**, **GPT Dictionary**, and **Post-Translation Dictionary**. Each category has both **Common** and **Project** variants.

As the names suggest, common dictionaries are visible to all projects, while project dictionaries are only visible to the current project.

To select which dictionaries a project uses, go to `Translation Settings -> Dictionary Settings` in the project.

Of these three categories, pre-translation and post-translation dictionaries are **replacement-type dictionaries**, while GPT dictionaries are **hint-type dictionaries**.

Pre-translation and post-translation dictionaries perform search-and-replace operations, while GPT dictionaries only serve to attach matching entries as additional context to the AI when the original text contains dictionary terms. Whether the AI uses these hints and how it applies them is beyond the program's control.


#### Dictionary Handling in GUI

* **File Mappings**
  * Common dictionaries can have multiple files, but each project has only one project dictionary and one name table.
  * GUI reads `NameTable.toml` from the project folder as the **Name Table** data.
  * Reads `Project PreDict.toml` as the **Project Pre-Translation Dictionary** data.
  * Reads `Project GptDict.toml` and `Project GptDict-Generated.toml` and merges their data as the **Project GPT Dictionary** data.
  * Reads `Project PostDict.toml` as the **Project Post-Translation Dictionary** data.

* **Edit Modes and Save Logic**
  * Name tables and dictionaries each have **Plain Text Mode** and **Table Mode**. Which mode's data is used during translation is determined when you click the Start Translation button.
  * For example, if the name table is displayed in table mode when you click Start Translation, the **Name Table (Table Mode)** data will first be saved to `NameTable.toml`, then translation executes. Editing in plain text mode without following TOML format will cause errors during translation.
  * Clicking **Refresh** reloads data from TOML files in the project folder. If you have unsaved modifications in the GUI, make sure to back them up first.
  * Clicking **Save** saves changes and refreshes the other mode's data. For example, editing in plain text mode then saving will also update the table mode view. Additionally, saving **Project GPT Dictionary** will **delete** `Project GptDict-Generated.toml` to prevent data duplication - please keep this in mind.

### A Typical Translation Workflow

1. New Project -> Enter project name.
2. Place files to translate in the `gt_input` folder within the new project directory.
3. Enter API credentials and key.
4. Use `GenDict` to auto-generate terminology glossary.
5. Adjust the glossary, modify dictionaries as needed, and select which dictionaries to use.
6. If the format supports name extraction, use `DumpName` and edit the name table.
7. Choose an appropriate production translation mode and translate.
8. Review issues.
9. Based on issues, edit `retranslKey`/`skipProblems`, rebuild/retranslate/modify cache as needed.
10. Collect results from `gt_output`.

### Cache File Structure

GalTransl++ cache may contain the following keys:

* `index`: Sequence index, generally not important.
* `name`: Character name, shows the preprocessed name (equivalent to pre_processed_name), empty if none.
* `names`: Multiple names, only seen in some VNT-extracted text.
* `name_preview`: The post-processed name to be output.
* `names_preview`: Same as above.
* `original_text`: Original text reference, identical to the original.
* `pre_processed_text`: The sentence's state after preprocessing, just before AI translation.
* `pre_translated_text`: The AI's raw response before any post-processing.
* `other_info`: Additional information.
* `problems`: Issues found during automated error checking.
* `translated_by`: The configured model for the API key used to translate this sentence.
* `translated_preview`: The final `message` to be output after all post-processing.

### Replacement Dictionary Syntax

* **Pre-Translation Dictionary** searches and replaces `original_text` to produce `pre_processed_text` for the AI.
* **Post-Translation Dictionary** searches and replaces `pre_translated_text` to produce the final `translated_preview`.

**Condition Target** specifies which text the condition regex applies to: can be any of `name`, `orig_text`, `preproc_text`, or `pretrans_text`.

When **Enable Regex** is `true`, source and target are treated as regular expressions for replacement. Higher priority dictionaries execute first.

## ⚙️ Processing and Translation Order

1. Optional: Apply pre-translation dictionary to `name` (name table collection also happens after this step).
2. Replace line breaks in `message` with `<br>`.
3. Replace tabs in `message` with `<tab>`.
4. Optional: Apply pre-translation dictionary to `message`.
5. Execute pre-processing plugins.
6. **AI Translation**.
7. Execute post-processing plugins.
8. Optional: Apply post-translation dictionary to `message`.
9. Replace `<tab>` back to tabs in `message`.
10. Replace `<br>` back to original line break symbols in `message`.
11. Optional: Apply GPT dictionary to `name` (GPT dictionary temporarily treated as replacement-type).
12. Apply name table replacement to `name` (entries with empty translations are ignored).
13. Optional: Apply post-translation dictionary to `name`.
14. Problem analysis.

> **Regex Engine Notes**
>
> Although GalTransl++'s core is written in C++, the regex engine uses `icu::Regex`, so all customizable regex operates on a character basis. For example, even though the emoji '😪' takes 4 bytes in UTF-8, it still matches a single `.` in regex.
>
> ICU extended regex syntax is also supported, such as using `\p{P}` to match any punctuation mark, `\p{Hangul}` to match any Korean character, etc.

## Additional Translation Notes

<details>

<summary>

### Problem Analysis Syntax Examples:

</summary>

retranslKeys syntax example

```
# Regex list - if any problem in the sentence cache matches any regex below, retranslate
# To retranslate based on specific source/target text, specify conditionTarget and conditionReg via inline table (array)
# Can also specify conditionScript and conditionFunc for external lua/py scripts (conditionFunc receives Sentence, returns bool)
# <PROJECT_DIR> is a macro representing the current project path string
# Prefix conditionTarget with prev_ or next_ to reference previous/next sentence, e.g., prev_prev_orig_text for two sentences back, condition fails if not found
retranslKeys = [
  #[{ conditionScript='<PROJECT_DIR>/Lua/MySampleTextPlugin.lua',conditionFunc='funcName'},
  #{ conditionScript='<PROJECT_DIR>/Python/MySampleTextPlugin.py',conditionFunc='funcName'}],
  #"Remaining Japanese",
  "Translation Failed", # Equivalent to [{ conditionTarget = 'problems', conditionReg = 'Translation Failed' }]
]
```

skipProblems syntax example

```
# Regex list - if any problem matches any regex below, it won't be added to the problems list
# To ignore specific problems for specific source/target text, specify conditionTarget and conditionReg via inline table (array)
# Put the problem to ignore (also regex) as the first item in the array
skipProblems = [
  # "^Introduced Latin: Live$"  # No conditions
  # If source contains 'モチモチ' and translation contains 'chewy', ignore all problems matching 'Introduced Latin' for this sentence
  [ 'Introduced Latin', { conditionTarget = 'preproc_text', conditionReg = 'モチモチ'},
    { conditionTarget = 'trans_preview', conditionReg = 'chewy'}],
]
```

Compare Object settings syntax example:

```
overwriteCompareObj = [
    { check = 'trans_preview', problemKey = 'Remaining Japanese' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = 'Punctuation Errors' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = 'Introduced Latin' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = 'Introduced Korean' },
    { base = 'orig_text', check = 'trans_preview', problemKey = 'Lost Line Break' },
    { base = 'orig_text', check = 'trans_preview', problemKey = 'Extra Line Break' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = 'Dict Unused' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = 'Language Mismatch' }
]
```
For example, `{ problemKey = 'Strictly Longer Than Source', base = 'orig_text', check = 'trans_preview' }`,

means when trans_preview is strictly longer than orig_text, record the corresponding problem.
</details>

<details>

<summary>

### EPUB Extraction and Custom Regex

</summary>

Given EPUB's diversity and GalTransl++'s project-centric philosophy, GalTransl++'s EPUB extraction doesn't have fixed parsing/assembly patterns like other translators.

GalTransl++ only uses the GUMBO library to traverse HTML/XHTML files within EPUBs
(EPUBs are just ZIP archives - you can open them with any decompression software and browse the contents).

Each string extracted from text tags becomes a single msg.

However, this alone creates obvious problems, like this sentence:
```
<p class="class_s2C-0">「モテる男は<ruby><rb>辛</rb><rt>つら</rt></ruby>いね」</p>
```
It would be split into four parts: 「モテる男は, 辛, つら, いね」, fed to the AI as 4 separate msgs. This is clearly not what we want. Using fixed HTML parsing methods or fixed regex groups for extraction either loses information or can't cover all cases - extremely low customizability. The output quality entirely depends on whether the program's parsing and assembly patterns happen to match user expectations. So GalTransl++ lets users build their own parsing patterns using regex based on their project needs. This raises the barrier slightly, but greatly increases flexibility.

EPUB regex settings still use TOML syntax and support two regex types: **Standard Regex** and **Callback Regex**.


#### Standard Regex

Standard regex performs simple match-and-replace. Using this preprocessing regex:
```
[[plugins.Epub.preprocRegex]]
org = '<ruby><rb>(.+?)</rb><rt>(.+?)</rt></ruby>'
rep = '[$2/$1]'
```
The sentence above becomes:
```
<p class="class_s2C-0">「モテる男は[つら/辛]いね」</p>
```
When traversing this location later, 「モテる男は\[つら/辛\]いね」 will be extracted as a complete sentence. Combined with this post-processing regex:
```
[[plugins.Epub.postprocRegex]]
org = '\[([^/\[\]]+?)/([^/\[\]]+?)\]'
rep = '<ruby><rb>$2</rb><rt>$1</rt></ruby>'
```
Ruby annotations can be restored at the desired location (you can modify prompts to tell the AI about the annotation format [furigana/base text] and preservation requirements for better results). If you only want to keep annotations in the source but remove them from the translation, simply add a de-annotation dictionary to the pre-translation dictionary. Many other customization needs can be met similarly.

#### Callback Regex

However, the above regex still has limitations. For these two sentences:
```
<p class="class_s2C-0">「うっ、レナ<span class="class_s2R">!?</span>」</p>
<p class="class_s2C-0">　二人の女の子が火花を散らす。どちらからの好意も嬉しくて、ついつい甘んじてし<span id="page_8"/>まう。</p>
```
If you want to "remove all tags inside `<p></p>` except the p tags themselves while keeping non-tag content" for quick filtering without examining each file's tags, simple regex (which doesn't handle nesting well) makes this difficult. This is where callback regex comes in.

Using this callback regex:
```
[[plugins.Epub.preprocRegex]]
org = '(<p[^>/]*>)(.*?)(</p>)'
callback = [ { group = 2, org = '<[^>]*>', rep = '' } ]
```
The two sentences become:
```
<p class="class_s2C-0">「うっ、レナ!?」</p>
<p class="class_s2C-0">　二人の女の子が火花を散らす。どちらからの好意も嬉しくて、ついつい甘んじてしまう。</p>
```

##### Callback Regex Processing Flow

Callback regex syntax: For each match, process all capture groups, applying the corresponding `group`'s callback regex/regex group for replacement, then concatenate all groups in order as the replacement string.

Using the first sentence as an example: `<p class="class_s2C-0">「うっ、レナ<span class="class_s2R">!?</span>」</p>` matches the regex `'(<p[^>/]*>)(.*?)(</p>)'`, so the entire sentence is one match. Based on capture groups (non-grouped parts are ignored):

*   **Group 1:** `<p class="class_s2C-0">`
*   **Group 2:** `「うっ、レナ<span class="class_s2R">!?</span>」`
*   **Group 3:** `</p>`

For group 1, the program looks for all callback regex with `group = 1`. The example has none, so group 1 outputs unchanged. Current replacement string: `<p class="class_s2C-0">`

For group 2, it finds `group = 2` regex, applies `org = '<[^>]*>', rep = ''` to get `「うっ、レナ!?」`. Current replacement string: `<p class="class_s2C-0">「うっ、レナ!?」`

Group 3 is like group 1, so the final replacement string is `<p class="class_s2C-0">「うっ、レナ!?」</p>`, matching expectations.
</details>

<details>

<summary>

### Python Module Usage and GPU Acceleration

</summary>
Since many deep learning libraries (tokenizers, PDF extractors, etc.) are only conveniently accessible from Python, this program ships with a small embedded Python environment by default - no manual download needed.

When Python libraries are needed (e.g., **translating PDFs** or **using Python-dependent tokenizers**), the program automatically downloads the required packages to the embedded environment.

However, you may need to **restart the program** afterward to reload the Python interpreter. Watch the log output window for prompts to avoid program freezes or crashes.

Using tokenizers like `spaCy's best trf model` or `Stanza` for full-text tokenization without GPU acceleration can be painfully slow. To enable GPU acceleration, follow this guide.

**Please ensure you have some technical ability and a decent graphics card!**

#### Enabling GPU Acceleration for `Stanza`

- 1. First ensure you have the **latest NVIDIA driver** for your GPU, then go to [NVIDIA CUDA Toolkit Archive](https://developer.nvidia.com/cuda-toolkit-archive) to **select an appropriate CUDA Toolkit version**, download and install the CUDA Toolkit Installer.
If unsure which CUDA version, run in cmd:
```cmd
nvidia-smi
```
to get the maximum CUDA version supported by your current driver (updating the driver first matters because different driver versions support different CUDA versions).
- 2. Visit [PyTorch website](https://pytorch.org/get-started/locally/), select your CUDA version to get the installation command.
If you forgot your installed CUDA version, run:
```cmd
nvcc --version
```
to get the current system CUDA version. Generally backward compatible, so any version should work.
- 3. Install PyTorch in the embedded environment. **You must use the Python from the embedded environment** (subsequent operations default to this environment).
Default location: `BaseConfig\python-3.12.10-embed-amd64`. Open cmd in this directory or always use the **absolute path** to python.exe to avoid confusion with any Python you may have installed (same for pip - must use env/python.exe -m pip...).
- 4. For example, if PyTorch gives you `pip3 install torch torchvision --index-url https://download.pytorch.org/whl/cu129`, open cmd in the above directory (type cmd in the path bar and press Enter) and run `python -m pip install torch torchvision --index-url https://download.pytorch.org/whl/cu129`.
Note: If torch is already installed, uninstall it first: `python -m pip uninstall torch`
- 5. Reinstall Stanza: `python -m pip uninstall stanza`, `python -m pip install stanza`
- 6. Try running `BaseConfig\pyScripts\check_stanza_gpu.py`. If it reports success, configuration is complete.
- 7. Open `BaseConfig\pyScripts\tokenizer_stanza.py`, change `use_gpu=False` to `use_gpu=True` in `self.nlp = stanza.Pipeline(lang=model_name, processors='tokenize,pos,ner', use_gpu=False, verbose=False)` to enable GPU acceleration for Stanza.

#### Enabling GPU Acceleration for `spaCy`

- 1. Same as Stanza steps 1 and 2.
- 2. In the **embedded Python environment (see Stanza step 3)**, reinstall spacy: `python -m pip uninstall spacy`, and ensure `cupy` is not installed.
- 3. Based on your CUDA version (see Stanza step 2), install the specific `cupy` version, e.g., `cupy-cuda13x`: `python -m pip install cupy-cuda13x`, then reinstall spacy: `python -m pip install spacy`.
- 4. Try running `BaseConfig\pyScripts\check_spacy_gpu.py`. If it reports success, configuration is complete.
- 5. Open `BaseConfig\pyScripts\tokenizer_spacy.py`, uncomment `#spacy.require_gpu()` to enable GPU acceleration for spaCy.

</details>

<details>

<summary>

### Lua/Python Embedding

</summary>
Both languages support hot-reloading within the program (as long as not occupied by other tasks). Changes take effect immediately on the next run.

For code examples, see `Example/Lua` and `Example/Python`. You'll need to review the source code for tool function/class function signatures.

</details>

## Additional Program Notes

<details>

<summary>

### Customizing Homepage Popular Cards

</summary>
See the `[[popularCards]]` array in (Example/)BaseConfig/globalConfig.toml for examples.

Minimum 6 cards required - fewer will be filled with program defaults. No maximum limit.

When `fromLocal` is `true` and `cardPixmap` is empty or missing, local file icons are automatically retrieved.

When `fromLocal` is `true`, the card button text becomes "Launch" and `pathOrUrl` paths are automatically converted for local launching.

However, the working directory doesn't change. Some programs that don't set their working directory to "program location" may have issues.

For such cases, write a redirect script:
```cmd
chcp 65001
cd /d %~dp0
start /b filename_to_run
```
Then launch this redirect script.

**Note:** Don't modify `globalConfig` while the program is running - it will be overwritten. Make changes after closing the program.
</details>

<details>

<summary>

### Tabs That Can Open in New Windows

</summary>

All GUI tab-style interfaces for dictionaries, name tables, prompts, etc. can be dragged out to open in new windows, as shown:

![tabView](img/tabView.png?raw=true)
</details>


## ⚙️ Build Guide

See [how-to-build.md](how-to-build.md)

## 🤝 Contributing

GalTransl++ is still in early stages for file format and plugin support. There may be other optimizations or bugs to address. Questions or suggestions are welcome via issues or PRs. Here's how to add file processors and plugins.

### Adding File Processors (Translator)

As you can see, GalTransl++'s core codebase has ~~fewer than twenty files~~ (probably more now), and interfaces are simple.

Since all file processors must inherit directly or indirectly from `ITranslator`, generally inherit `NormalJsonTranslator` directly unless there's a special reason.

Then you just need to extract files to JSON, set the extraction folder as `NormalJsonTranslator`'s `inputDir`,
and set the expected translated JSON output folder as its `outputDir`.

When a file finishes translating (if single-file splitting is used, only after file merging), it calls the member function object `m_onFileProcessed` (thread-safe).

Use this to determine file write-back timing. See `EpubTranslator` for examples.

Factory function to register: `createTranslator`.


### Adding Plugin Processors (Plugin)

Text processing plugins are theoretically divided into pre-processing and post-processing plugins. GalTransl++ doesn't accept plugins intended to work on both pre and post-translation - that would break custom ordering of plugin processing. If needed, please split your plugin into Pre and Post versions.

All plugins only need to satisfy the `PPlugin` constraint.

* **For pre-processing plugins**: If non-filtering, only modify `pre_processed_text`. If filtering, you can set `complete` to `true` and be responsible for filling `pre_translated_text`. If needed, you can also set `notAnalyzeProblem` to `true` to prevent problem analysis for this sentence.
* **For post-processing plugins**: Only modify `translated_preview`.

Any plugin can insert keys into `other_info` to attach information in the cache.

See `TextLinebreakFix` and `TextPostFull2Half` for examples.

Factory function to register: `registerPlugins`.


### Other Notes

Since my development environment is basically tied to Windows and I don't have Linux devices, even though the project uses few easily-replaceable Windows APIs,

I won't proactively consider cross-platform support.

Also, since I use a newer environment, there may be some rare issues.

Currently known: The `mecab:x64-windows` dependency doesn't compile under VS2026 (toolset v145), but VS2022 (toolset v143) works. You may need to switch back to VS2022 when installing dependencies.
