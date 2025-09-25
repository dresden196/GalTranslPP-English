# GalTransl++

![GalTransl++ GUI](img/GalTranslPP.png?raw=true)

**GalTransl++** 是继承了 [GalTransl](https://github.com/GalTransl/GalTransl)  `以项目为本`的主要理念及架构，凝练其两年间积累的精华部分，同时吸收了大量Gal补丁作者经验而进行优化的，轻量透明的、拥有高度且方便的扩展能力的翻译核心。

**GalTransl++ GUI** 是包装了GalTransl++核心的，以 `在GUI下尽可能保持高度自定义` 为目标的，Fluent UI风格的交互界面，也是本项目的重点开发对象。

## ✨ 特性

本项目在继承了GalTransl基本功能的基础上，包括但不限于对以下模块进行了优化：

* 更好的单文件分割缓存命中
* 优先标点的换行修复
* 可选(带icu扩展的)正则形式的，高度自定义的译前译后字典和明确的优先级
* 高度自定义的epub提取
* 有效的API额度耗尽检测
* 卡片弹出式的完成提示 (仅GUI)
* 更好的字典未使用检测
* 可自定义的符号检测
* 单独生成用以检查的预处理结果
* 统合生成的翻译问题概览
* 速度更快的rebuild
* 更加方便的提示词自定义
* 更清晰的字典使用设定
* 重翻时附带已知问题
* 可自定义的问题比较对象

![notification](img/notification.png?raw=true)

## 📖 流程说明

### GalTransl++ 翻译流程与替换型字典介绍

对于熟悉GalTransl的人来说，过渡到GalTransl++ CLI版本可谓易如反掌，所以接下来主要讲GUI。

这里还是再稍微谈一下翻译流程吧(默认你已经读过GalTransl的使用说明，翻译接口什么的就略过了)。

GalTransl++无论处理哪种文件格式，最后都是统一化为json来读取。

每个json文件是一个对象列表，每个对象代表一个『句子』，这是喂给AI进行翻译的基本单位。

读取json时主要关注两个键，分别为 `name` 和 `message`，不同的翻译模式(`transEngine`)就是对这两个键进行各种不同的处理：


#### 翻译模式 (transEngine)

* **`# ForGalJson`**: 实际翻译模式，向AI输入json格式的句子(包含`name`和`message`)并要求AI以json格式回复，程序将解析返回的Json。
* **`# ForGalTsv`**: 实际翻译模式，向AI输入TSV格式的句子(包含`name`和`message`)并要求AI以TSV格式回复，程序将解析返回的TSV，可能比ForGalJson模式更省token。
* **`# ForNovelTsv`**: 实际翻译模式，和 `ForGalTsv` 的区别主要是变动提示词，向AI输入和解析的时候都不带`name`键。
* **`# DeepseekJson`**: 实际翻译模式，和 `ForGalJson` 的唯一区别是程序自带的默认提示词变成了中文。
* **`# Sakura`**: 实际翻译模式，向AI输入自然语言形式的句子(包含`name`和`message`)，由于Sakura是翻译特化模型，不必要求即会返回同样形式的的句子，程序解析返回的自然语言。
* **`# DumpName`**: 提取所有的 `name` 键，在项目文件夹下生成 `人名替换表.toml` 以供统一替换人名。
* **`# GenDict`**: 借助AI自动生成术语表，保存在项目文件夹下的 `项目GPT字典-生成.toml` 中。
* **`# Rebuild`**: 即使 `problem` 或 `orig_text` 中包含 `retranslKey` 也不会重翻，只根据缓存重建结果。
* **`# ShowNormal`**: 保存预处理后的内容及句子到项目文件夹下带 `show_normal` 字段的文件夹中，如Epub格式下可生成预处理后的html/xhtml文件以及生成的json，可用于检查和排错。

### 缓存机制

在`Rebuild`中所提到的缓存，是指翻译过后留存在项目文件夹下，`trans_cache`文件夹中的json文件，其中按顺序存储了每个文件对应的序列号，原文和译文等信息。所有的实际翻译模式都会先读取缓存，然后只挑选出缓存中还没有的原文进行翻译。

> **⚠️ 特别注意**：在使用单文件分割功能的情况下，由于缓存命中结合了上下文，所以当你改变文件本身，或者分割数/分割方式时，会有一部分无关的句子不能命中缓存。理论上文件切的越碎，最终分割出的文件份数比最大线程数超过的更多，则不能命中缓存的句子越多。GalTransl++会尽可能在这种情况下保证原有缓存的命中，不过如果希望达到更好的缓存命中，最好还是不改变分割方式和分割数。为此也可以使用 `ShowNormal` 模式观察切割后的文件。

`retranslKey`指的是重翻关键字，`problem`指的是缓存中的问题。

GalTransl++会在翻译时自动分析翻译时常见问题，并将问题输出到缓存中。

一般情况下，如果 `problem` 或 `orig_text`(一个缓存键，存储的是原始message) 中包含设定的`retranslKey`，则即使在实际翻译模式下命中缓存，这个句子依然会被重翻。所以如果只想重建缓存，要么得删除所有`retranslKey`，要么使用 `Rebuild` 模式忽略翻译。

### 字典系统

GalTransl++的字典分为 **译前字典**，**GPT字典**，**译后字典** 三大类。每类字典都有 **通用** 和 **项目** 两种。

顾名思义，通用字典可以被所有项目所见，项目字典只能被当前项目所见。

具体一个项目要用哪些字典，可以在项目的 `翻译设置->字典设置` 中找到对应的选项进行选择。


这三类字典中，译前和译后字典为 **替换型字典**，GPT字典为 **提示型字典**。

即译前和译后字典执行的是搜索替换，而GPT字典的作用仅在于当原文中出现字典里的词时，将该条字典作为附加部分一并喂给AI以规范翻译，那至于AI想不想用，用成什么样，就不是程序能管得到的了。


#### GUI中的字典处理

* **文件对应关系**
  * 通用字典可以有多个，而项目字典和人名表每个项目各只有一个。
  * GUI会读取项目文件夹下 `人名替换表.toml` 来作为 **人名表** 中的数据。
  * 读取 `项目字典_译前.toml` 来作为 **项目译前字典** 中的数据。
  * 读取 `项目GPT字典.toml` 和 `项目GPT字典-生成.toml` 并合并其中的数据来作为 **项目GPT字典** 中的数据。
  * 读取 `项目字典_译后.toml` 来作为 **项目译后字典** 中的数据。

* **编辑模式与保存逻辑**
  * 人名表和字典都分别有 **纯文本模式** 和 **表模式**，具体在翻译时用哪个模式的数据会在你按开始翻译按钮时决定。
  * 例如，你在按开始翻译按钮时，人名表是以表模式显示的，则会先将 **人名表(表模式)** 中的数据保存到 `人名替换表.toml` 中，然后再执行翻译。如果在纯文本模式下没有按 toml 格式来编辑，翻译时肯定会报错。
  * 按 **刷新** 将会重新从项目文件夹中的 toml 文件读取数据。如果你在GUI中还有修改了没有保存的数据，请务必先确认备份情况再刷新。
  * 按 **保存** 会在保存的同时刷新另一模式的数据。比如在纯文本模式中编辑后按下保存，则此时表模式也会更新刚刚编辑过的内容。另外，保存 **项目GPT字典** 时会 **删除**  `项目GPT字典-生成.toml` 以防止数据重复，请务必注意。

> **⚠️ 注意**：由于`DumpName`/`GenDict`任务会分别生成 `人名替换表.toml` / `项目GPT字典-生成.toml` 并覆盖原有文件，默认也会在任务结束后自动刷新，所以请务必注意不要被其覆盖掉有用的信息。

### 一个常见的翻译流程

1、  新建项目 -> 输入项目名。  
2、  在新建的项目文件夹中的`gt_input`文件夹中放入待翻译的文件。  
3、  填入API和key。  
4、  使用 `GenDict` 自动生成术语表。  
5、  调整术语表，根据需求修改字典并选择要使用的字典。  
6、  如果文件支持提取name，则可 `DumpName` 并编辑人名表。  
7、  选用合适的实际翻译模式进行翻译。  
8、  查看问题。  
9、  根据问题选择编辑`retranslKey`并重翻 / 直接修改缓存。  
10、 重翻 / 重建。  
11、 在`gt_output`中查收结果。  

### 缓存文件结构

GalTransl++的缓存中可能包含如下键:

* `index`: 索引顺序，一般不重要。
* `name`: 人名，展示的是预处理后的人名(相当于pre_processed_name)，如果没有则为空。
* `name_preview`: 将输出的译后人名。
* `original_text`: 原文对照，与原文没有任何区别。
* `pre_processed_text`: 经过一系列预处理后，即将执行AI翻译前时句子的样子。
* `pre_translated_text`: AI返回的，未经过任何后处理的句子的样子。
* `problem`: 在自动化找错中找出的问题。
* `translated_by`: 翻译此句子所用的 apikey 的设定模型。
* `translated_preview`: 经过所有后处理之后，将输出的 message。
* `other_info`: 其它附加信息。

### 替换型字典语法

* **译前字典** 会搜索并替换 `original_text` 以输出 `pre_processed_text` 提供给AI。
* **译后字典** 会搜索并替换 `pre_translated_text` 以供 `translated_preview` 最终输出。

**条件对象** 是指条件正则要作用于的文本，可以是 `name`, `orig_text`, `preproc_text`, `pretrans_text` 中的任意一个。

当 **启用正则** 为 `true` 时，原文和译文将被视为正则表达式进行替换，优先级越高的字典越先执行。

## ⚙️ 处理与翻译顺序

1、  可选: 对 `name` 执行译前字典替换 (人名表收集的也是这一步后的 `name`)。  
2、  将 `message` 中的换行统一替换为 `<br>`。  
3、  将 `message` 中的制表符统一替换为 `<tab>`。  
4、  可选: 对 `message` 执行译前字典替换。  
5、  执行前处理插件。  
6、  **AI翻译**。  
7、  执行后处理插件。  
8、  可选: 对 `message` 执行译后字典替换。  
9、  将 `message` 中的 `<tab>` 统一替换回制表符。  
10、 将 `message` 中的换行从 `<br>` 换回原符号。  
11、 可选: 对 `name` 执行GPT字典替换 (此时GPT字典将被临时视为替换型字典)。  
12、 对 `name` 执行人名表替换 (人名表译名为空则忽略)。  
13、 可选: 对 `name` 执行译后字典替换。  
14、 问题分析。  

> **正则引擎说明**
>
> 虽然GalTransl++的核心为C++编写，但由于正则引擎采用`icu::Regex`，所以可自定义编辑的所有正则依然是以字符为单位的。比如即使emoji表情'😪' 在UTF-8中占用4个字节，在正则中依然是以一个`.`来匹配的。
>
> 同时也支持icu扩展正则语法，如可以使用`\p{P}`来匹配任意标点符号，`\p{Hangul}`来匹配任意韩文字符等。

## 其它翻译说明

<details>
  <summary>
### retranslKey语法示例:
  </summary>
```
retranslKeys = [
  "翻译失败",
  "残留日文",
  "本有",
  "本无",
  # ...
]
```

问题比较对象设定语法示例:

```
overwriteCompareObj = [
    { base = 'orig_text', check = 'trans_preview', problemKey = '词频过高' },
    { check = 'trans_preview', problemKey = '残留日文' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = '标点错漏' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = '引入拉丁字母' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = '引入韩文' },
    { base = 'orig_text', check = 'trans_preview', problemKey = '丢失换行' },
    { base = 'orig_text', check = 'trans_preview', problemKey = '多加换行' },
    { base = 'orig_text', check = 'trans_preview', problemKey = '比原文长' },
    { base = 'orig_text', check = 'trans_preview', problemKey = '比原文长严格' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = '字典未使用' },
    { base = 'preproc_text', check = 'trans_preview', problemKey = '语言不通' }
]
```
如 `{ base = 'orig_text', check = 'trans_preview', problemKey = '比原文长严格' }`， 

意思是当 trans\_preview 比 orig\_text 严格长时，在 problem 中留下对应的问题。
</details>

<details>
  <summary>
### 📘 Epub 提取与正则自定义
  </summary>

鉴于Epub的多样性以及GalTransl++以项目为本的理念，GalTransl++的Epub提取并不像其它翻译器一样有固定的解析/拼装模式。

GalTransl++将仅使用GUMBO库遍历Epub中的HTML/XHTML文件
(epub文件本身只是一个zip包，你可以使用任何解压软件打开它并浏览其中的文件)，

每个文本标签提取出的字符串都将作为一个msg。

但仅仅这样会有一些明显的问题，如下面这个句子:
```
<p class="class_s2C-0">「モテる男は<ruby><rb>辛</rb><rt>つら</rt></ruby>いね」</p>
```
它会被拆解为「モテる男は 辛 つら いね」四个部分，作为4个msg喂给AI，这显然不是我们想要的。如果使用固定的html解析方法/固定的正则组进行提取，要么会丢失信息，要么无法兼顾所有情况，总之自定义程度极低，最后输出的效果理想与否完全取决于程序的解析与拼装模式是否正好符合用户的预期。所以GalTransl++选择让用户根据自己的项目自行利用正则构建解析模式，这显然增长了一些门槛，不过作为回报大幅提升了自由度。

Epub正则设置依然使用toml语法解析配置，并支持两种正则写法: **一般正则**和**回调正则**。


#### 一般正则

一般正则执行简单的匹配替换，例如使用下面的预处理正则，
```
[[plugins.Epub.'预处理正则']]
org = '<ruby><rb>(.+?)</rb><rt>(.+?)</rt></ruby>'
rep = '[$2/$1]'
```
则上面的句子会被替换为:
```
<p class="class_s2C-0">「モテる男は[つら/辛]いね」</p>
```
之后再遍历到此处时，「モテる男は\[つら/辛\]いね」 将被作为一个完整的句子被提取出来。如果之后搭配如下后处理正则，
```
[[plugins.Epub.'后处理正则']]
org = '\[([^/\[\]]+?)/([^/\[\]]+?)\]'
rep = '<ruby><rb>$2</rb><rt>$1</rt></ruby>'
```
则可在理想位置还原振假名效果(可以修改提示词告诉AI注音格式\[振假名/基本文本\]及保留要求来达到更好的保留效果)。当然如果只想在原文中保留振假名而在译文中删去，那也很简单，直接在译前字典中再写一个去注音字典即可。其它很多自定义需求也都可借此满足。

#### 回调正则

不过如上正则仍有一些缺陷，比如这两句话:
```
<p class="class_s2C-0">「うっ、レナ<span class="class_s2R">!?</span>」</p>
<p class="class_s2C-0">　二人の女の子が火花を散らす。どちらからの好意も嬉しくて、ついつい甘んじてし<span id="page_8"/>まう。</p>
```
假如我不想一个一个文件的看标签写正则，而是想要直接『删除所有`<p></p>`标签内的其余标签并保留非标签部分』来快速过滤的话，面对不擅长处理嵌套的简单正则，显然我们很难写出这样的正则/正则组来处理这个问题，那么此时就需要使用回调正则。

我们可以编写如下回调正则，
```
[[plugins.Epub.'预处理正则']]
org = '(<p[^>/]*>)(.*?)(</p>)'
callback = [ { group = 2, org = '<[^>]*>', rep = '' } ]
```
则这两句话会被替换为
```
<p class="class_s2C-0">「うっ、レナ!?」</p>
<p class="class_s2C-0">　二人の女の子が火花を散らす。どちらからの好意も嬉しくて、ついつい甘んじてしまう。</p>
```

##### 回调正则处理流程

回调正则的语法是，处理每个匹配项中的所有捕获组，对每个捕获组使用其对应`group`的回调正则/正则组进行替换，最后将所有捕获组按顺序合并作为这个匹配项的替换字符串。

采用第一句话作为例子，第一句话 `<p class="class_s2C-0">「うっ、レナ<span class="class_s2R">!?</span>」</p>` 整句被正则 `'(<p[^>/]*>)(.*?)(</p>)'` 匹配到，则这一整句作为一个匹配项。在这个匹配项中，根据捕获组又可以分为三组(没有被分组的部分则直接忽略)：

*   **第一组:** `<p class="class_s2C-0">`
*   **第二组:** `「うっ、レナ<span class="class_s2R">!?</span>」`
*   **第三组:** `</p>`

对于第一组文本，程序会寻找所有 `group = 1` 的回调正则，并依次执行替换。显然上面的回调正则并没有 `group = 1` 的正则，则忽略替换，第一组原样输出，现在的替换字符串暂定为: `<p class="class_s2C-0">`

对于第二组文本，程序找到了 `group = 2` 的正则，则对第二组文本使用 `org = '<[^>]*>', rep = ''` 进行替换，替换后得到 `「うっ、レナ!?」` ，则现在的替换字符串暂定为 `<p class="class_s2C-0">「うっ、レナ!?」`

第三组文本与第一组文本同理，则最终的替换字符串为 `<p class="class_s2C-0">「うっ、レナ!?」</p>` ，与预期相符。
</details>

<details>
  <summary>
### PDF翻译说明
  </summary>
与Epub不同，GalTransl++将PDF转为NormalJson是通过调用外部脚本完成的，因为目前能较好重建PDF的只有python库。

此脚本需要能通过CLI参数对指定pdf执行提取/回注操作，具体参数详见[PDFTranslator](GalTranslPP/PDFTranslator.ixx)

示例脚本为 (Example/)BaseConfig/PDFConverter/PDFConverter.py

显而易见地，运行此脚本需要python环境(推荐3.12)并安装 `babeldoc` 库，
```cmd
pip install babeldoc
```
Release里也提供了此脚本打包后的exe，如果不想或者不会配置环境的可以[点此下载](https://github.com/julixian/GalTranslPP/releases/download/v1.0.2/PDFConverter.zip)(不过非常大就是了)。
</details>


## ⚙️ 编译指南

详见[how-to-build.md](how-to-build.md)

## 🤝 贡献指南

GalTransl++在文件支持和插件支持上仍处于起步阶段，也不排除有其它优化的思路或需要修改的bug，如果有疑问或建议，欢迎提出 issue 或 PR。接下来主要说一下如何添加文件处理器/插件。

### 添加文件处理器 (Translator)

如你所见，GalTransl++的核心代码文件数量不超过二十个，接口也十分简单。

由于所有的文件处理器需直接/间接继承自 `ITranslator`，如无特殊情况，一般直接继承 `NormalJsonTranslator` 即可。

这样只需要将相应文件提取为json，并重设提取文件夹为 `NormalJsonTranlator` 的 `inputDir` 文件夹，

重设期望获取译后json的文件夹为其 `outputDir` 文件夹即可。

每当其翻译完一个文件时(如果有单文件分割则只在文件合并后)，其会调用成员变量中的函数对象 `m_onFileProcessed`(线程安全)，

可以借此来判断文件的写回时机，具体示例可见 `EpubTranslator`。

需要注册的工厂函数为 `createTranslator`。


### 添加插件处理器 (Plugin)

文本处理插件理论上分为前处理插件和后处理插件，GalTransl++不接受希望同时在译前和译后都生效的插件，那样会破坏对于插件处理顺序的自定义性。如果有需要请将您的插件分为 Pre版 和 Post版。

所有插件均需继承自 `IPlugin`，并实现 `run` 方法，仅此即可。

* **对于译前插件**：如果是非过滤型插件，原则上只允许修改 `pre_processed_text`。如果是过滤型插件，可以将 `complete` 置为 `true`，并负责填充 `pre_translated_text`。如有需要，也可以在 `other_info` 中插入信息键，以便之后在 `problemAnalyzer` 中过滤带此键的问题分析等。
* **对于译后插件**：原则上只允许修改 `translated_preview`。

任意插件均可在 `other_info` 中插入键以在缓存中附带信息。

具体示例可见 `TextLinebreakFix` 和 `TextPostFull2Half`。

需要注册的工厂函数为 `registerPlugins`。


### 其它注意事项

由于我的开发环境基本绑定 windows系统，我自己也没有linux设备，所以即使在项目中使用的winapi数量很少也很好替换，

但是跨平台的事我自己是不会主动考虑的。

另外由于我所使用的环境较新，也可能会有一些比较罕见的问题。

目前已知项目依赖 `mecab:x64-windows` 在VS2026(工具集 v145)下不过编，但是VS2022(工具集 v143)能过，安装依赖可能需要切回VS2022  
