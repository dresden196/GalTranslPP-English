#include "TLFCfgPage.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDesktopServices>

#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaComboBox.h"
#include "ElaToggleSwitch.h"
#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaToolTip.h"

import Tool;

TLFCfgPage::TLFCfgPage(toml::ordered_value& projectConfig, QWidget* parent) : BasePage(parent), _projectConfig(projectConfig)
{
	setWindowTitle(tr("换行修复设置"));
	setContentsMargins(10, 0, 10, 0);

	// 创建一个中心部件和布局
	QWidget* centerWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(centerWidget);

	// 换行模式
	QStringList fixModes = { "优先标点", "保持位置", "固定字数", "平均" };
	QStringList fixModesToShow = { tr("优先标点"), tr("保持位置"), tr("固定字数"), tr("平均") };
	QString fixMode = QString::fromStdString(toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "换行模式", ""));
	ElaScrollPageArea* fixModeArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* fixModeLayout = new QHBoxLayout(fixModeArea);
	ElaText* fixModeText = new ElaText(tr("换行模式"), fixModeArea);
	fixModeText->setTextPixelSize(16);
	fixModeLayout->addWidget(fixModeText);
	fixModeLayout->addStretch();
	ElaComboBox* fixModeComboBox = new ElaComboBox(fixModeArea);
	fixModeComboBox->addItems(fixModesToShow);
	if (!fixMode.isEmpty()) {
		int index = fixModes.indexOf(fixMode);
		if (index >= 0) {
			fixModeComboBox->setCurrentIndex(index);
		}
	}
	fixModeLayout->addWidget(fixModeComboBox);
	mainLayout->addWidget(fixModeArea);

	// 仅在标点后添加
	bool onlyAddAfterPunct = toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "仅在标点后添加", true);
	ElaScrollPageArea* onlyAddAfterPunctArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* onlyAddAfterPunctLayout = new QHBoxLayout(onlyAddAfterPunctArea);
	ElaText* onlyAddAfterPunctText = new ElaText(tr("仅在标点后添加"), onlyAddAfterPunctArea);
	ElaToolTip* onlyAddAfterPunctTip = new ElaToolTip(onlyAddAfterPunctArea);
	onlyAddAfterPunctTip->setToolTip(tr("仅在优先标点模式有效"));
	onlyAddAfterPunctText->setTextPixelSize(16);
	onlyAddAfterPunctLayout->addWidget(onlyAddAfterPunctText);
	onlyAddAfterPunctLayout->addStretch();
	ElaToggleSwitch* onlyAddAfterPunctToggleSwitch = new ElaToggleSwitch(onlyAddAfterPunctArea);
	onlyAddAfterPunctToggleSwitch->setIsToggled(onlyAddAfterPunct);
	onlyAddAfterPunctLayout->addWidget(onlyAddAfterPunctToggleSwitch);
	mainLayout->addWidget(onlyAddAfterPunctArea);

	// 分段字数阈值
	int threshold = toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "分段字数阈值", 21);
	ElaScrollPageArea* segmentThresholdArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* segmentThresholdLayout = new QHBoxLayout(segmentThresholdArea);
	ElaText* segmentThresholdText = new ElaText(tr("分段字数阈值"), segmentThresholdArea);
	ElaToolTip* segmentThresholdTip = new ElaToolTip(segmentThresholdArea);
	segmentThresholdTip->setToolTip(tr("仅在固定字数模式有效"));
	segmentThresholdText->setTextPixelSize(16);
	segmentThresholdLayout->addWidget(segmentThresholdText);
	segmentThresholdLayout->addStretch();
	ElaSpinBox* segmentThresholdSpinBox = new ElaSpinBox(segmentThresholdArea);
	segmentThresholdSpinBox->setRange(1, 999);
	segmentThresholdSpinBox->setValue(threshold);
	segmentThresholdLayout->addWidget(segmentThresholdSpinBox);
	mainLayout->addWidget(segmentThresholdArea);

	// 强制修复
	bool forceFix = toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "强制修复", false);
	ElaScrollPageArea* forceFixArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* forceFixLayout = new QHBoxLayout(forceFixArea);
	ElaText* forceFixText = new ElaText(tr("强制修复"), forceFixArea);
	forceFixText->setTextPixelSize(16);
	forceFixLayout->addWidget(forceFixText);
	forceFixLayout->addStretch();
	ElaToggleSwitch* forceFixToggleSwitch = new ElaToggleSwitch(forceFixArea);
	forceFixToggleSwitch->setIsToggled(forceFix);
	forceFixLayout->addWidget(forceFixToggleSwitch);
	mainLayout->addWidget(forceFixArea);

	// 报错阈值
	int errorThreshold = toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "报错阈值", 28);
	ElaScrollPageArea* errorThresholdArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* errorThresholdLayout = new QHBoxLayout(errorThresholdArea);
	ElaText* errorThresholdText = new ElaText(tr("报错阈值"), errorThresholdArea);
	ElaToolTip* errorThresholdTip = new ElaToolTip(errorThresholdArea);
	errorThresholdTip->setToolTip(tr("单行字符数超过此阈值时报错"));
	errorThresholdText->setTextPixelSize(16);
	errorThresholdLayout->addWidget(errorThresholdText);
	errorThresholdLayout->addStretch();
	ElaSpinBox* errorThresholdSpinBox = new ElaSpinBox(errorThresholdArea);
	errorThresholdSpinBox->setRange(1, 999);
	errorThresholdSpinBox->setValue(errorThreshold);
	errorThresholdLayout->addWidget(errorThresholdSpinBox);
	mainLayout->addWidget(errorThresholdArea);

	mainLayout->addSpacing(15);
	ElaText* tokenizerConfigText = new ElaText(tr("分词器设置"), this);
	tokenizerConfigText->setWordWrap(false);
	tokenizerConfigText->setTextPixelSize(18);
	mainLayout->addWidget(tokenizerConfigText);

	mainLayout->addSpacing(10);

	// 使用分词器
	bool useTokenizer = toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "使用分词器", false);
	ElaScrollPageArea* useTokenizerArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* useTokenizerLayout = new QHBoxLayout(useTokenizerArea);
	ElaText* useTokenizerText = new ElaText(tr("使用分词器"), useTokenizerArea);
	ElaToolTip* useTokenizerTip = new ElaToolTip(useTokenizerArea);
	useTokenizerTip->setToolTip(tr("可能可以获得更好的换行效果(不过感觉用处不大)"));
	useTokenizerText->setTextPixelSize(16);
	useTokenizerLayout->addWidget(useTokenizerText);
	useTokenizerLayout->addStretch();
	ElaToggleSwitch* useTokenizerToggleSwitch = new ElaToggleSwitch(useTokenizerArea);
	useTokenizerToggleSwitch->setIsToggled(useTokenizer);
	useTokenizerLayout->addWidget(useTokenizerToggleSwitch);
	mainLayout->addWidget(useTokenizerArea);

	// tokenizerBackend
	QStringList tokenizerBackends = { "MeCab", "spaCy", "Stanza" };
	QString tokenizerBackend = QString::fromStdString(toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "tokenizerBackend", "MeCab"));
	ElaScrollPageArea* tokenizerBackendArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* tokenizerBackendLayout = new QHBoxLayout(tokenizerBackendArea);
	ElaText* tokenizerBackendText = new ElaText(tr("分词器后端"), tokenizerBackendArea);
	ElaToolTip* tokenizerBackendTip = new ElaToolTip(tokenizerBackendArea);
	tokenizerBackendTip->setToolTip(tr("应选择适合目标语言的后端/模型/字典"));
	tokenizerBackendText->setTextPixelSize(16);
	tokenizerBackendLayout->addWidget(tokenizerBackendText);
	tokenizerBackendLayout->addStretch();
	ElaComboBox* tokenizerBackendComboBox = new ElaComboBox(tokenizerBackendArea);
	tokenizerBackendComboBox->addItems(tokenizerBackends);
	if (!tokenizerBackend.isEmpty()) {
		int index = tokenizerBackends.indexOf(tokenizerBackend);
		if (index >= 0) {
			tokenizerBackendComboBox->setCurrentIndex(index);
		}
	}
	tokenizerBackendLayout->addWidget(tokenizerBackendComboBox);
	mainLayout->addWidget(tokenizerBackendArea);

	// mecabDictDir
	QString mecabDictDir = QString::fromStdString(toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "mecabDictDir", "BaseConfig/mecabDict/mecab-chinese"));
	ElaScrollPageArea* mecabDictDirArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* mecabDictDirLayout = new QHBoxLayout(mecabDictDirArea);
	ElaText* mecabDictDirText = new ElaText(tr("MeCab词典目录"), mecabDictDirArea);
	ElaToolTip* mecabDictDirTip = new ElaToolTip(mecabDictDirText);
	mecabDictDirTip->setToolTip(tr("MeCab中文词典需手动下载"));
	mecabDictDirText->setTextPixelSize(16);
	mecabDictDirLayout->addWidget(mecabDictDirText);
	mecabDictDirLayout->addStretch();
	ElaLineEdit* mecabDictDirLineEdit = new ElaLineEdit(mecabDictDirArea);
	mecabDictDirLineEdit->setFixedWidth(400);
	mecabDictDirLineEdit->setText(mecabDictDir);
	mecabDictDirLayout->addWidget(mecabDictDirLineEdit);
	mainLayout->addWidget(mecabDictDirArea);

	// spaCyModelName https://spacy.io/models
	QString spaCyModelName = QString::fromStdString(toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "spaCyModelName", "zh_core_web_trf"));
	ElaScrollPageArea* spaCyModelNameArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* spaCyModelNameLayout = new QHBoxLayout(spaCyModelNameArea);
	ElaText* spaCyModelNameText = new ElaText(tr("spaCy模型名称"), spaCyModelNameArea);
	ElaToolTip* spaCyModelNameTip = new ElaToolTip(spaCyModelNameText);
	spaCyModelNameTip->setToolTip(tr("spaCy模型名称，新模型下载后需重启程序"));
	spaCyModelNameText->setTextPixelSize(16);
	spaCyModelNameLayout->addWidget(spaCyModelNameText);
	spaCyModelNameLayout->addStretch();
	ElaLineEdit* spaCyModelNameLineEdit = new ElaLineEdit(spaCyModelNameArea);
	spaCyModelNameLineEdit->setFixedWidth(200);
	spaCyModelNameLineEdit->setText(spaCyModelName);
	spaCyModelNameLayout->addWidget(spaCyModelNameLineEdit);
	ElaPushButton* browseSpaCyModelButton = new ElaPushButton(tr("浏览"), spaCyModelNameArea);
	browseSpaCyModelButton->setToolTip(tr("浏览模型目录"));
	spaCyModelNameLayout->addWidget(browseSpaCyModelButton);
	connect(browseSpaCyModelButton, &ElaPushButton::clicked, this, [=]()
		{
			QDesktopServices::openUrl(QUrl("https://spacy.io/models"));
		});
	mainLayout->addWidget(spaCyModelNameArea);

	// Stanza https://stanfordnlp.github.io/stanza/ner_models.html
	QString stanzaLang = QString::fromStdString(toml::find_or(_projectConfig, "plugins", "TextLinebreakFix", "stanzaLang", "zh"));
	ElaScrollPageArea* stanzaArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* stanzaLayout = new QHBoxLayout(stanzaArea);
	ElaText* stanzaText = new ElaText(tr("Stanza语言ID"), stanzaArea);
	ElaToolTip* stanzaTip = new ElaToolTip(stanzaText);
	stanzaTip->setToolTip(tr("Stanza语言ID，新模型下载后需重启程序"));
	stanzaText->setTextPixelSize(16);
	stanzaLayout->addWidget(stanzaText);
	stanzaLayout->addStretch();
	ElaLineEdit* stanzaLineEdit = new ElaLineEdit(stanzaArea);
	stanzaLineEdit->setFixedWidth(200);
	stanzaLineEdit->setText(stanzaLang);
	stanzaLayout->addWidget(stanzaLineEdit);
	ElaPushButton* browseStanzaModelButton = new ElaPushButton(tr("浏览"), stanzaArea);
	browseStanzaModelButton->setToolTip(tr("浏览模型目录"));
	stanzaLayout->addWidget(browseStanzaModelButton);
	connect(browseStanzaModelButton, &ElaPushButton::clicked, this, [=]()
		{
			QDesktopServices::openUrl(QUrl("https://stanfordnlp.github.io/stanza/ner_models.html"));
		});
	mainLayout->addWidget(stanzaArea);


	_applyFunc = [=]()
		{
			insertToml(_projectConfig, "plugins.TextLinebreakFix.换行模式", fixModes[fixModeComboBox->currentIndex()].toStdString());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.仅在标点后添加", onlyAddAfterPunctToggleSwitch->getIsToggled());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.分段字数阈值", segmentThresholdSpinBox->value());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.强制修复", forceFixToggleSwitch->getIsToggled());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.报错阈值", errorThresholdSpinBox->value());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.使用分词器", useTokenizerToggleSwitch->getIsToggled());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.tokenizerBackend", tokenizerBackends[tokenizerBackendComboBox->currentIndex()].toStdString());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.mecabDictDir", mecabDictDirLineEdit->text().toStdString());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.spaCyModelName", spaCyModelNameLineEdit->text().toStdString());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.stanzaLang", stanzaLineEdit->text().toStdString());
		};

	mainLayout->addStretch();
	centerWidget->setWindowTitle(tr("换行修复设置"));
	addCentralWidget(centerWidget, true, true, 0);
}

TLFCfgPage::~TLFCfgPage()
{

}
