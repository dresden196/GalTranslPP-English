#include "TLFCfgPage.h"

#include <QVBoxLayout>
#include <QFormLayout>

#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaComboBox.h"
#include "ElaToggleSwitch.h"
#include "ElaText.h"
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
	QString fixMode = QString::fromStdString(toml::get_or(_projectConfig["plugins"]["TextLinebreakFix"]["换行模式"], ""));
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
	bool onlyAddAfterPunct = toml::get_or(_projectConfig["plugins"]["TextLinebreakFix"]["仅在标点后添加"], true);
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
	int threshold = toml::get_or(_projectConfig["plugins"]["TextLinebreakFix"]["分段字数阈值"], 21);
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
	bool forceFix = toml::get_or(_projectConfig["plugins"]["TextLinebreakFix"]["强制修复"], false);
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
	int errorThreshold = toml::get_or(_projectConfig["plugins"]["TextLinebreakFix"]["报错阈值"], 35);
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

	_applyFunc = [=]()
		{
			insertToml(_projectConfig, "plugins.TextLinebreakFix.换行模式", fixModes[fixModeComboBox->currentIndex()].toStdString());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.仅在标点后添加", onlyAddAfterPunctToggleSwitch->getIsToggled());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.分段字数阈值", segmentThresholdSpinBox->value());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.强制修复", forceFixToggleSwitch->getIsToggled());
			insertToml(_projectConfig, "plugins.TextLinebreakFix.报错阈值", errorThresholdSpinBox->value());
		};

	mainLayout->addStretch();
	centerWidget->setWindowTitle(tr("换行修复设置"));
	addCentralWidget(centerWidget);
}

TLFCfgPage::~TLFCfgPage()
{

}
