#include "CommonSettingsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QDesktopServices>
#include <QFileDialog>

#include "ElaText.h"
#include "ElaPlainTextEdit.h"
#include "ElaMessageBar.h"
#include "ElaDrawerArea.h"
#include "ElaLineEdit.h"
#include "ElaComboBox.h"
#include "ElaScrollPageArea.h"
#include "ElaRadioButton.h"
#include "ElaSpinBox.h"
#include "ElaToggleSwitch.h"
#include "ElaPushButton.h"
#include "ElaToolTip.h"

import Tool;

CommonSettingsPage::CommonSettingsPage(toml::ordered_value& projectConfig, QWidget* parent) : BasePage(parent), _projectConfig(projectConfig)
{
	setWindowTitle(tr("一般设置"));
	setTitleVisible(false);

	_setupUI();
}

CommonSettingsPage::~CommonSettingsPage()
{

}

void CommonSettingsPage::_setupUI()
{
	QWidget* mainWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);

	// 单次请求翻译句子数量
	int requestNum = toml::find_or(_projectConfig, "common", "numPerRequestTranslate", 8);
	ElaScrollPageArea* requestNumArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* requestNumLayout = new QHBoxLayout(requestNumArea);
	ElaText* requestNumText = new ElaText(tr("单次请求翻译句子数量"), requestNumArea);
	requestNumText->setWordWrap(false);
	requestNumText->setTextPixelSize(16);
	ElaToolTip* requestNumTip = new ElaToolTip(requestNumText);
	requestNumTip->setToolTip(tr("推荐值 < 15"));
	requestNumLayout->addWidget(requestNumText);
	requestNumLayout->addStretch();
	ElaSpinBox* requestNumSpinBox = new ElaSpinBox(requestNumArea);
	requestNumSpinBox->setFocus();
	requestNumSpinBox->setRange(1, 100);
	requestNumSpinBox->setValue(requestNum);
	requestNumLayout->addWidget(requestNumSpinBox);
	mainLayout->addWidget(requestNumArea);

	// 最大线程数
	int maxThread = toml::find_or(_projectConfig, "common", "threadsNum", 1);
	ElaScrollPageArea* maxThreadArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* maxThreadLayout = new QHBoxLayout(maxThreadArea);
	ElaText* maxThreadText = new ElaText(tr("最大线程数"), maxThreadArea);
	maxThreadText->setTextPixelSize(16);
	maxThreadLayout->addWidget(maxThreadText);
	maxThreadLayout->addStretch();
	ElaSpinBox* maxThreadSpinBox = new ElaSpinBox(maxThreadArea);
	maxThreadSpinBox->setRange(1, 100);
	maxThreadSpinBox->setValue(maxThread);
	maxThreadLayout->addWidget(maxThreadSpinBox);
	mainLayout->addWidget(maxThreadArea);

	// 翻译顺序，name为文件名，size为大文件优先，多线程时大文件优先可以提高整体速度[name/size]
	std::string order = toml::find_or(_projectConfig, "common", "sortMethod", "size");
	ElaScrollPageArea* orderArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* orderLayout = new QHBoxLayout(orderArea);
	ElaText* orderText = new ElaText(tr("翻译顺序"), orderArea);
	orderText->setTextPixelSize(16);
	ElaToolTip* orderTip = new ElaToolTip(orderText);
	orderTip->setToolTip(tr("name为文件名，size为大文件优先，多线程时大文件优先可以提高整体速度"));
	orderLayout->addWidget(orderText);
	orderLayout->addStretch();
	QButtonGroup* orderGroup = new QButtonGroup(orderArea);
	ElaRadioButton* orderNameRadio = new ElaRadioButton(tr("文件名"), orderArea);
	orderNameRadio->setChecked(order == "name");
	orderLayout->addWidget(orderNameRadio);
	ElaRadioButton* orderSizeRadio = new ElaRadioButton(tr("文件大小"), orderArea);
	orderSizeRadio->setChecked(order == "size");
	orderLayout->addWidget(orderSizeRadio);
	orderGroup->addButton(orderNameRadio, 0);
	orderGroup->addButton(orderSizeRadio, 1);
	mainLayout->addWidget(orderArea);

	// 翻译到的目标语言，包括但不限于[zh-cn/zh-tw/en/ja/ko/ru/fr]
	std::string target = toml::find_or(_projectConfig, "common", "targetLang", "zh-cn");
	QString targetStr = QString::fromStdString(target);
	ElaScrollPageArea* targetArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* targetLayout = new QHBoxLayout(targetArea);
	ElaText* targetText = new ElaText(tr("翻译到的目标语言"), targetArea);
	targetText->setTextPixelSize(16);
	ElaToolTip* targetTip = new ElaToolTip(targetText);
	targetTip->setToolTip(tr("包括但不限于[zh-cn/zh-tw/en/ja/ko/ru/fr]"));
	targetLayout->addWidget(targetText);
	targetLayout->addStretch();
	ElaLineEdit* targetLineEdit = new ElaLineEdit(targetArea);
	targetLineEdit->setFixedWidth(150);
	targetLineEdit->setText(targetStr);
	targetLayout->addWidget(targetLineEdit);
	mainLayout->addWidget(targetArea);

	// 是否启用单文件分割。Num: 每n条分割一次，Equal: 每个文件均分n份，No: 关闭单文件分割。[No/Num/Equal]
	std::string split = toml::find_or(_projectConfig, "common", "splitFile", "No");
	ElaDrawerArea* splitArea = new ElaDrawerArea(mainWidget);
	QWidget* splitHeaderWidget = new QWidget(splitArea);
	splitArea->setDrawerHeader(splitHeaderWidget);
	QHBoxLayout* splitLayout = new QHBoxLayout(splitHeaderWidget);
	ElaText* splitText = new ElaText(tr("单文件分割"), splitHeaderWidget);
	splitText->setTextPixelSize(16);
	ElaToolTip* splitTip = new ElaToolTip(splitText);
	splitTip->setToolTip(tr("Num: 每n条分割一次，Equal: 每个文件均分n份，No: 关闭单文件分割。"));
	splitLayout->addWidget(splitText);
	splitLayout->addStretch();
	QButtonGroup* splitGroup = new QButtonGroup(splitHeaderWidget);
	ElaRadioButton* splitNoRadio = new ElaRadioButton("No", splitHeaderWidget);
	splitNoRadio->setChecked(split == "No");
	splitLayout->addWidget(splitNoRadio);
	ElaRadioButton* splitNumRadio = new ElaRadioButton("Num", splitHeaderWidget);
	splitNumRadio->setChecked(split == "Num");
	splitLayout->addWidget(splitNumRadio);
	ElaRadioButton* splitEqualRadio = new ElaRadioButton("Equal", splitHeaderWidget);
	splitEqualRadio->setChecked(split == "Equal");
	splitLayout->addWidget(splitEqualRadio);
	splitGroup->addButton(splitNoRadio, 0);
	splitGroup->addButton(splitNumRadio, 1);
	splitGroup->addButton(splitEqualRadio, 2);
	connect(splitGroup, &QButtonGroup::buttonToggled, this, [=](QAbstractButton* button, bool checked)
		{
			if (checked) {
				std::string value = button->text().toStdString();
				if (value == "No") {
					splitArea->collapse();
				}
				else {
					splitArea->expand();
				}
			}
		});
	connect(splitArea, &ElaDrawerArea::expandStateChanged, this, [=](bool expanded)
		{
			if (expanded && splitGroup->button(0)->isChecked()) {
				splitArea->collapse();
			}
		});
	mainLayout->addWidget(splitArea);

	// Num时，表示n句拆分一次；Equal时，表示每个文件均分拆成n部分。
	int splitNum = toml::find_or(_projectConfig, "common", "splitFileNum", 1024);
	QWidget* splitNumArea = new QWidget(splitArea);
	QHBoxLayout* splitNumLayout = new QHBoxLayout(splitNumArea);
	ElaText* splitNumText = new ElaText(tr("分割数量"), splitNumArea);
	splitNumText->setTextPixelSize(16);
	ElaToolTip* splitNumTip = new ElaToolTip(splitNumText);
	splitNumTip->setToolTip(tr("Num时，表示n句拆分一次；Equal时，表示每个文件均分拆成n部分。"));
	splitNumLayout->addWidget(splitNumText);
	splitNumLayout->addStretch();
	ElaSpinBox* splitNumSpinBox = new ElaSpinBox(splitNumArea);
	splitNumSpinBox->setRange(1, 10000);
	splitNumSpinBox->setValue(splitNum);
	splitNumLayout->addWidget(splitNumSpinBox);
	splitArea->addDrawer(splitNumArea);
	if (split == "No") {
		splitArea->collapse();
	}
	else {
		splitArea->expand();
	}

	// 每翻译n次保存一次缓存
	int saveInterval = toml::find_or(_projectConfig, "common", "saveCacheInterval", 1);
	ElaScrollPageArea* cacheArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* cacheLayout = new QHBoxLayout(cacheArea);
	ElaText* cacheText = new ElaText(tr("缓存保存间隔"), cacheArea);
	cacheText->setTextPixelSize(16);
	ElaToolTip* cacheTip = new ElaToolTip(cacheText);
	cacheTip->setToolTip(tr("每翻译n次保存一次缓存"));
	cacheLayout->addWidget(cacheText);
	cacheLayout->addStretch();
	ElaSpinBox* cacheSpinBox = new ElaSpinBox(cacheArea);
	cacheSpinBox->setRange(1, 10000);
	cacheSpinBox->setValue(saveInterval);
	cacheLayout->addWidget(cacheSpinBox);
	mainLayout->addWidget(cacheArea);

	// 最大重试次数
	int maxRetries = toml::find_or(_projectConfig, "common", "maxRetries", 5);
	ElaScrollPageArea* retryArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* retryLayout = new QHBoxLayout(retryArea);
	ElaText* retryText = new ElaText(tr("最大重试次数"), retryArea);
	retryText->setTextPixelSize(16);
	retryLayout->addWidget(retryText);
	retryLayout->addStretch();
	ElaSpinBox* retrySpinBox = new ElaSpinBox(retryArea);
	retrySpinBox->setRange(1, 100);
	retrySpinBox->setValue(maxRetries);
	retryLayout->addWidget(retrySpinBox);
	mainLayout->addWidget(retryArea);

	// 携带上文数量
	int contextNum = toml::find_or(_projectConfig, "common", "contextHistorySize", 8);
	ElaScrollPageArea* contextArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* contextLayout = new QHBoxLayout(contextArea);
	ElaText* contextText = new ElaText(tr("携带上文数量"), contextArea);
	contextText->setTextPixelSize(16);
	contextLayout->addWidget(contextText);
	contextLayout->addStretch();
	ElaSpinBox* contextSpinBox = new ElaSpinBox(contextArea);
	contextSpinBox->setRange(1, 100);
	contextSpinBox->setValue(contextNum);
	contextLayout->addWidget(contextSpinBox);
	mainLayout->addWidget(contextArea);

	// 智能重试  # 解析结果失败时尝试折半重翻与清空上下文，避免无效重试。
	bool smartRetry = toml::find_or(_projectConfig, "common", "smartRetry", true);
	ElaScrollPageArea* smartRetryArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* smartRetryLayout = new QHBoxLayout(smartRetryArea);
	ElaText* smartRetryText = new ElaText(tr("智能重试"), smartRetryArea);
	smartRetryText->setTextPixelSize(16);
	ElaToolTip* smartRetryTip = new ElaToolTip(smartRetryText);
	smartRetryTip->setToolTip(tr("解析结果失败时尝试折半重翻与清空上下文，避免无效重试。"));
	smartRetryLayout->addWidget(smartRetryText);
	smartRetryLayout->addStretch();
	ElaToggleSwitch* smartRetryToggle = new ElaToggleSwitch(smartRetryArea);
	smartRetryToggle->setIsToggled(smartRetry);
	smartRetryLayout->addWidget(smartRetryToggle);
	mainLayout->addWidget(smartRetryArea);

	// 额度检测 # 运行时动态检测key额度
	bool checkQuota = toml::find_or(_projectConfig, "common", "checkQuota", true);
	ElaScrollPageArea* checkQuotaArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* checkQuotaLayout = new QHBoxLayout(checkQuotaArea);
	ElaText* checkQuotaText = new ElaText(tr("额度检测"), checkQuotaArea);
	checkQuotaText->setTextPixelSize(16);
	ElaToolTip* checkQuotaTip = new ElaToolTip(checkQuotaText);
	checkQuotaTip->setToolTip(tr("运行时动态检测key额度"));
	checkQuotaLayout->addWidget(checkQuotaText);
	checkQuotaLayout->addStretch();
	ElaToggleSwitch* checkQuotaToggle = new ElaToggleSwitch(checkQuotaArea);
	checkQuotaToggle->setIsToggled(checkQuota);
	checkQuotaLayout->addWidget(checkQuotaToggle);
	mainLayout->addWidget(checkQuotaArea);

	// 项目日志级别
	std::string logLevel = toml::find_or(_projectConfig, "common", "logLevel", "info");
	QString logLevelStr = QString::fromStdString(logLevel);
	ElaScrollPageArea* logArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* logLayout = new QHBoxLayout(logArea);
	ElaText* logText = new ElaText(tr("日志级别"), logArea);
	logText->setTextPixelSize(16);
	logLayout->addWidget(logText);
	logLayout->addStretch();
	ElaComboBox* logComboBox = new ElaComboBox(logArea);
	logComboBox->addItem("trace");
	logComboBox->addItem("debug");
	logComboBox->addItem("info");
	logComboBox->addItem("warn");
	logComboBox->addItem("err");
	logComboBox->addItem("critical");
	if (!logLevelStr.isEmpty()) {
		int index = logComboBox->findText(logLevelStr);
		if (index >= 0) {
			logComboBox->setCurrentIndex(index);
		}
	}
	logLayout->addWidget(logComboBox);
	mainLayout->addWidget(logArea);

	// 保存项目日志
	bool saveLog = toml::find_or(_projectConfig, "common", "saveLog", true);
	ElaScrollPageArea* saveLogArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* saveLogLayout = new QHBoxLayout(saveLogArea);
	ElaText* saveLogText = new ElaText(tr("保存项目日志"), saveLogArea);
	saveLogText->setTextPixelSize(16);
	saveLogLayout->addWidget(saveLogText);
	saveLogLayout->addStretch();
	ElaToggleSwitch* saveLogToggle = new ElaToggleSwitch(saveLogArea);
	saveLogToggle->setIsToggled(saveLog);
	saveLogLayout->addWidget(saveLogToggle);
	mainLayout->addWidget(saveLogArea);

	mainLayout->addSpacing(15);
	ElaText* tokenizerConfigText = new ElaText(tr("分词器设置"), this);
	tokenizerConfigText->setTextPixelSize(18);
	ElaToolTip* tokenizerConfigTip = new ElaToolTip(tokenizerConfigText);
	tokenizerConfigTip->setToolTip(tr("用于生成字典和查错的分词器后端及其设置(应选择适合原文的后端/模型/字典)"));
	mainLayout->addWidget(tokenizerConfigText);

	// tokenizerBackend
	std::string tokenizerBackend = toml::find_or(_projectConfig, "common", "tokenizerBackend", "MeCab");
	ElaScrollPageArea* tokenizerBackendArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* tokenizerBackendLayout = new QHBoxLayout(tokenizerBackendArea);
	ElaText* tokenizerBackendText = new ElaText(tr("分词器后端"), tokenizerBackendArea);
	tokenizerBackendText->setTextPixelSize(16);
	ElaToolTip* tokenizerBackendTip = new ElaToolTip(tokenizerBackendText);
	tokenizerBackendTip->setToolTip(tr("除了MeCab，剩下的都依赖Python，所以速度变慢或内存占用变大是正常的"));
	tokenizerBackendLayout->addWidget(tokenizerBackendText);
	tokenizerBackendLayout->addStretch();
	ElaComboBox* tokenizerBackendComboBox = new ElaComboBox(tokenizerBackendArea);
	tokenizerBackendComboBox->addItem("MeCab");
	tokenizerBackendComboBox->addItem("spaCy");
	tokenizerBackendComboBox->addItem("Stanza");
	if (int index = tokenizerBackendComboBox->findText(QString::fromStdString(tokenizerBackend)); index >= 0) {
		tokenizerBackendComboBox->setCurrentIndex(index);
	}
	tokenizerBackendLayout->addWidget(tokenizerBackendComboBox);
	mainLayout->addWidget(tokenizerBackendArea);

	// mecabDictDir
	std::string mecabDictDir = toml::find_or(_projectConfig, "common", "mecabDictDir", "BaseConfig/mecabDict/mecab-ipadic-utf8");
	ElaScrollPageArea* mecabDictDirArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* mecabDictDirLayout = new QHBoxLayout(mecabDictDirArea);
	ElaText* mecabDictDirText = new ElaText(tr("MeCab词典目录"), mecabDictDirArea);
	ElaToolTip* mecabDictDirTip = new ElaToolTip(mecabDictDirText);
	mecabDictDirTip->setToolTip(tr("MeCab词典目录，程序自带一个"));
	mecabDictDirText->setTextPixelSize(16);
	mecabDictDirLayout->addWidget(mecabDictDirText);
	mecabDictDirLayout->addStretch();
	ElaLineEdit* mecabDictDirLineEdit = new ElaLineEdit(mecabDictDirArea);
	mecabDictDirLineEdit->setFixedWidth(400);
	mecabDictDirLineEdit->setText(QString::fromStdString(mecabDictDir));
	mecabDictDirLayout->addWidget(mecabDictDirLineEdit);
	mainLayout->addWidget(mecabDictDirArea);
	ElaPushButton* mecabDictDirButton = new ElaPushButton(tr("浏览"), mecabDictDirArea);
	mecabDictDirLayout->addWidget(mecabDictDirButton);
	connect(mecabDictDirButton, &QPushButton::clicked, this, [=]()
		{
			QString dir = QFileDialog::getExistingDirectory(this, tr("选择MeCab词典目录"), mecabDictDirLineEdit->text());
			if (!dir.isEmpty()) {
				mecabDictDirLineEdit->setText(dir);
			}
		});

	// spaCyModelName  https://spacy.io/models
	std::string spaCyModelName = toml::find_or(_projectConfig, "common", "spaCyModelName", "ja_core_news_trf");
	ElaScrollPageArea* spaCyModelNameArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* spaCyModelNameLayout = new QHBoxLayout(spaCyModelNameArea);
	ElaText* spaCyModelNameText = new ElaText(tr("spaCy模型名称"), spaCyModelNameArea);
	ElaToolTip* spaCyModelNameTip = new ElaToolTip(spaCyModelNameText);
	spaCyModelNameTip->setToolTip(tr("spaCy模型名称，新模型下载后需重启程序"));
	spaCyModelNameText->setTextPixelSize(16);
	spaCyModelNameLayout->addWidget(spaCyModelNameText);
	spaCyModelNameLayout->addStretch();
	ElaLineEdit* spaCyModelNameLineEdit = new ElaLineEdit(spaCyModelNameArea);
	spaCyModelNameLineEdit->setFixedWidth(200);
	spaCyModelNameLineEdit->setText(QString::fromStdString(spaCyModelName));
	spaCyModelNameLayout->addWidget(spaCyModelNameLineEdit);
	mainLayout->addWidget(spaCyModelNameArea);
	ElaPushButton* spaCyModelNameButton = new ElaPushButton(tr("浏览"), spaCyModelNameArea);
	spaCyModelNameLayout->addWidget(spaCyModelNameButton);
	connect(spaCyModelNameButton, &QPushButton::clicked, this, [=]()
		{
			QDesktopServices::openUrl(QUrl("https://spacy.io/models"));
		});

	// stanzaLang https://stanfordnlp.github.io/stanza/ner_models.html
	std::string stanzaLang = toml::find_or(_projectConfig, "common", "stanzaLang", "ja");
	ElaScrollPageArea* stanzaLangArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* stanzaLangLayout = new QHBoxLayout(stanzaLangArea);
	ElaText* stanzaLangText = new ElaText(tr("Stanza语言ID"), stanzaLangArea);
	ElaToolTip* stanzaLangTip = new ElaToolTip(stanzaLangText);
	stanzaLangTip->setToolTip(tr("Stanza语言ID，新模型下载后需重启程序"));
	stanzaLangText->setTextPixelSize(16);
	stanzaLangLayout->addWidget(stanzaLangText);
	stanzaLangLayout->addStretch();
	ElaLineEdit* stanzaLangLineEdit = new ElaLineEdit(stanzaLangArea);
	stanzaLangLineEdit->setFixedWidth(200);
	stanzaLangLineEdit->setText(QString::fromStdString(stanzaLang));
	stanzaLangLayout->addWidget(stanzaLangLineEdit);
	mainLayout->addWidget(stanzaLangArea);
	ElaPushButton* stanzaLangButton = new ElaPushButton(tr("浏览"), stanzaLangArea);
	stanzaLangLayout->addWidget(stanzaLangButton);
	connect(stanzaLangButton, &QPushButton::clicked, this, [=]()
		{
			QDesktopServices::openUrl(QUrl("https://stanfordnlp.github.io/stanza/ner_models.html"));
		});


	// 项目所用的换行
	std::string linebreakSymbol = toml::find_or(_projectConfig, "common", "linebreakSymbol", "auto");
	toml::ordered_value lbsVal = linebreakSymbol;
	lbsVal.as_string_fmt().fmt = toml::string_format::basic;
	QString linebreakSymbolStr = QString::fromStdString(toml::format(toml::ordered_value{ toml::ordered_table{{"linebreakSymbol", lbsVal}} }));
	mainLayout->addSpacing(15);
	ElaText* linebreakText = new ElaText(tr("本项目所使用的换行符"), mainWidget);
	linebreakText->setTextPixelSize(18);
	ElaToolTip* linebreakTip = new ElaToolTip(linebreakText);
	linebreakTip->setToolTip(tr("将换行符统一规范为 &lt;br&gt; 以方便检错和修复，也可以让如全角半角转化等插件方便忽略换行。<br>具体替换时机详见使用说明，auto为自动检测"));
	mainLayout->addWidget(linebreakText);
	ElaPlainTextEdit* linebreakEdit = new ElaPlainTextEdit(mainWidget);
	linebreakEdit->setPlainText(linebreakSymbolStr);
	linebreakEdit->setFixedHeight(100);
	mainLayout->addWidget(linebreakEdit);


	_applyFunc = [=]()
		{
			insertToml(_projectConfig, "common.numPerRequestTranslate", requestNumSpinBox->value());
			insertToml(_projectConfig, "common.threadsNum", maxThreadSpinBox->value());
			int orderValue = orderGroup->id(orderGroup->checkedButton());
			QString orderValueStr;
			if (orderValue == 0) {
				orderValueStr = "name";
			}
			else if (orderValue == 1) {
				orderValueStr = "size";
			}
			insertToml(_projectConfig, "common.sortMethod", orderValueStr.toStdString());
			insertToml(_projectConfig, "common.targetLang", targetLineEdit->text().toStdString());
			insertToml(_projectConfig, "common.splitFile", splitGroup->checkedButton()->text().toStdString());
			insertToml(_projectConfig, "common.splitFileNum", splitNumSpinBox->value());
			insertToml(_projectConfig, "common.saveCacheInterval", cacheSpinBox->value());
			insertToml(_projectConfig, "common.maxRetries", retrySpinBox->value());
			insertToml(_projectConfig, "common.contextHistorySize", contextSpinBox->value());
			insertToml(_projectConfig, "common.smartRetry", smartRetryToggle->getIsToggled());
			insertToml(_projectConfig, "common.checkQuota", checkQuotaToggle->getIsToggled());
			insertToml(_projectConfig, "common.logLevel", logComboBox->currentText().toStdString());
			insertToml(_projectConfig, "common.saveLog", saveLogToggle->getIsToggled());

			insertToml(_projectConfig, "common.tokenizerBackend", tokenizerBackendComboBox->currentText().toStdString());
			insertToml(_projectConfig, "common.mecabDictDir", mecabDictDirLineEdit->text().toStdString());
			insertToml(_projectConfig, "common.spaCyModelName", spaCyModelNameLineEdit->text().toStdString());
			insertToml(_projectConfig, "common.stanzaLang", stanzaLangLineEdit->text().toStdString());
			try {
				toml::ordered_value newTbl = toml::parse_str<toml::ordered_type_config>(linebreakEdit->toPlainText().toStdString());
				auto& newLinebreakSymbol = newTbl["linebreakSymbol"];
				if (newLinebreakSymbol.is_string()) {
					insertToml(_projectConfig, "common.linebreakSymbol", newLinebreakSymbol);
				}
				else {
					insertToml(_projectConfig, "common.linebreakSymbol", "auto");
				}
			}
			catch (...) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("解析失败"), tr("linebreakSymbol不符合 toml 规范"), 3000);
			}
		};

	mainLayout->addStretch();
	addCentralWidget(mainWidget, true, true, 0);
}