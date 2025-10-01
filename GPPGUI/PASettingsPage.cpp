#include "PASettingsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QButtonGroup>
#include <QFileDialog>

#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaScrollPageArea.h"
#include "ElaToggleSwitch.h"
#include "ElaToolTip.h"
#include "ElaMessageBar.h"
#include "ElaSlider.h"
#include "ElaPushButton.h"
#include "ElaDoubleSpinBox.h"
#include "ElaPlainTextEdit.h"
#include "ElaToggleButton.h"
#include "ValueSliderWidget.h"
#include "ElaFlowLayout.h"

import Tool;

PASettingsPage::PASettingsPage(toml::ordered_value& projectConfig, QWidget* parent) : BasePage(parent), _projectConfig(projectConfig)
{
	setWindowTitle(tr("问题分析"));
	setTitleVisible(false);

	_setupUI();
}

PASettingsPage::~PASettingsPage()
{

}

void PASettingsPage::_setupUI()
{
	QWidget* mainWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(5);

	// 要发现的问题清单
	QStringList problemList = { "词频过高","标点错漏","丢失换行","多加换行","比原文长","比原文长严格","字典未使用",
		"残留日文","引入拉丁字母","引入韩文","语言不通", };
	QStringList problemListToShow = { tr("词频过高"), tr("标点错漏"), tr("丢失换行"), tr("多加换行"), tr("比原文长"), tr("比原文长严格"),
		tr("字典未使用"), tr("残留日文"), tr("引入拉丁字母"), tr("引入韩文"), tr("语言不通"), };

	std::set<std::string> problemListSet;
	auto& problemListOrgArray = _projectConfig["problemAnalyze"]["problemList"];
	if (problemListOrgArray.is_array()) {
		for (const auto& problem : problemListOrgArray.as_array()) {
			if (problem.is_string()) {
				problemListSet.insert(problem.as_string());
			}
		}
	}
	ElaText* problemListTitle = new ElaText(tr("要发现的问题清单"), mainWidget);
	problemListTitle->setTextPixelSize(18);
	mainLayout->addWidget(problemListTitle);
	ElaFlowLayout* problemListLayout = new ElaFlowLayout();
	QList<ElaToggleButton*> problemListButtons;
	for (int i = 0; i < problemList.size(); i++) {
		ElaToggleButton* button = new ElaToggleButton(problemListToShow[i], this);
		button->setFixedWidth(120);
		if (problemListSet.contains(problemList[i].toStdString())) {
			button->setIsToggled(true);
		}
		problemListLayout->addWidget(button);
		problemListButtons.append(button);
	}
	mainLayout->addLayout(problemListLayout);
	mainLayout->addSpacing(20);

	// 规定标点错漏要查哪些标点
	std::string punctuationSet = toml::get_or(_projectConfig["problemAnalyze"]["punctSet"], "");
	QString punctuationSetStr = QString::fromStdString(punctuationSet);
	ElaScrollPageArea* punctuationListArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* punctuationListLayout = new QHBoxLayout(punctuationListArea);
	ElaText* punctuationListTitle = new ElaText(tr("标点查错"), punctuationListArea);
	punctuationListTitle->setTextPixelSize(16);
	ElaToolTip* punctuationListTip = new ElaToolTip(punctuationListTitle);
	punctuationListTip->setToolTip(tr("规定标点错漏要查哪些标点"));
	punctuationListLayout->addWidget(punctuationListTitle);
	punctuationListLayout->addStretch();
	ElaLineEdit* punctuationList = new ElaLineEdit(punctuationListArea);
	punctuationList->setText(punctuationSetStr);
	punctuationListLayout->addWidget(punctuationList);
	mainLayout->addWidget(punctuationListArea);

	// 语言不通检测的语言置信度，设置越高则检测越精准，但可能遗漏，反之亦然
	double languageProbability = toml::get_or(_projectConfig["problemAnalyze"]["langProbability"], 0.85);
	ElaScrollPageArea* languageProbabilityArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* languageProbabilityLayout = new QHBoxLayout(languageProbabilityArea);
	ElaText* languageProbabilityTitle = new ElaText(tr("语言置信度"), languageProbabilityArea);
	languageProbabilityTitle->setTextPixelSize(16);
	ElaToolTip* languageProbabilityTip = new ElaToolTip(languageProbabilityTitle);
	languageProbabilityTip->setToolTip(tr("语言不通检测的语言置信度(0-1)，设置越高则检测越精准，但可能遗漏，反之亦然"));
	languageProbabilityLayout->addWidget(languageProbabilityTitle);
	languageProbabilityLayout->addStretch();
	ValueSliderWidget* languageProbabilitySlider = new ValueSliderWidget(languageProbabilityArea);
	languageProbabilitySlider->setFixedWidth(500);
	languageProbabilitySlider->setValue(languageProbability);
	languageProbabilityLayout->addWidget(languageProbabilitySlider);
	mainLayout->addWidget(languageProbabilityArea);

	mainLayout->addSpacing(20);

	// 重翻在缓存的problem或pre_jp中包含对应**关键字**的句子，去掉下面列表中的#号注释来使用，也可添加自定义的关键字。
	toml::ordered_value retranslKeysArr = toml::array{};
	auto& retranslKeys = _projectConfig["problemAnalyze"]["retranslKeys"];
	if (retranslKeys.is_array()) {
		retranslKeysArr = retranslKeys.as_array();
	}
	retranslKeysArr.as_array_fmt().fmt = toml::array_format::multiline;
	ElaText* retranslKeyHelperText = new ElaText(tr("重翻关键字设定"), mainWidget);
	ElaToolTip* retranslKeyHelperTip = new ElaToolTip(retranslKeyHelperText);
	retranslKeyHelperTip->setToolTip(tr("重翻在缓存的problem或orig_text中包含对应 **关键字** 的句子，遵循toml格式"));
	retranslKeyHelperText->setTextPixelSize(18);
	retranslKeyHelperText->setWordWrap(false);
	mainLayout->addWidget(retranslKeyHelperText);
	ElaPlainTextEdit* retranslKeyEdit = new ElaPlainTextEdit(mainWidget);
	QFont font = retranslKeyEdit->font();
	font.setPixelSize(14);
	retranslKeyEdit->setFont(font);
	retranslKeyEdit->setPlainText(QString::fromStdString(toml::format(toml::ordered_value{ toml::ordered_table{{ "retranslKeys", retranslKeysArr }} })));
	retranslKeyEdit->moveCursor(QTextCursor::End);
	mainLayout->addWidget(retranslKeyEdit);

	mainLayout->addSpacing(20);

	// 问题的比较对象和被比较对象(不写则默认为orig_text和transPreview)
	toml::ordered_value compareObjArr = toml::array{};
	auto& overwriteCompareObj = _projectConfig["problemAnalyze"]["overwriteCompareObj"];
	if (overwriteCompareObj.is_array()) {
		compareObjArr = overwriteCompareObj.as_array();
	}
	compareObjArr.as_array_fmt().fmt = toml::array_format::multiline;
	ElaText* compareObjHelperText = new ElaText(tr("问题比较对象设定"), mainWidget);
	ElaToolTip* compareObjHelperTip = new ElaToolTip(compareObjHelperText);
	compareObjHelperTip->setToolTip(tr("问题的比较对象和被比较对象(不写则默认为orig_text和transPreview)，遵循toml格式"));
	compareObjHelperText->setTextPixelSize(18);
	compareObjHelperText->setWordWrap(false);
	mainLayout->addWidget(compareObjHelperText);
	ElaPlainTextEdit* compareObjEdit = new ElaPlainTextEdit(mainWidget);
	compareObjEdit->setMinimumHeight(300);
	compareObjEdit->setFont(font);
	compareObjEdit->setPlainText(QString::fromStdString(toml::format(toml::ordered_value{ toml::ordered_table{{ "overwriteCompareObj", compareObjArr }} })));
	compareObjEdit->moveCursor(QTextCursor::End);
	mainLayout->addWidget(compareObjEdit);

	QWidget* illusButtonWidget = new QWidget(mainWidget);
	QHBoxLayout* illusButtonLayout = new QHBoxLayout(illusButtonWidget);
	illusButtonLayout->addStretch();
	ElaPushButton* illusButton = new ElaPushButton(tr("语法示例"), illusButtonWidget);
	ElaToolTip* illusButtonTip = new ElaToolTip(illusButton);
	illusButtonTip->setToolTip(tr("查看 重翻关键字 和 问题比较对象 设定的语法示例"));
	illusButtonLayout->addWidget(illusButton);
	connect(illusButton, &ElaPushButton::clicked, this, [=]()
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile("BaseConfig/illustration/pahelper.html"));
		});
	mainLayout->addWidget(illusButtonWidget);
	
	mainLayout->addStretch();

	_applyFunc = [=]()
		{
			toml::array problemListArray;
			for (qsizetype i = 0; i < problemListButtons.size(); i++) {
				if (!problemListButtons[i]->getIsToggled()) {
					continue;
				}
				problemListArray.push_back(problemList[i].toStdString());
			}
			insertToml(_projectConfig, "problemAnalyze.problemList", problemListArray);
			insertToml(_projectConfig, "problemAnalyze.punctSet", punctuationList->text().toStdString());
			insertToml(_projectConfig, "problemAnalyze.langProbability", languageProbabilitySlider->value());

			try {
				toml::ordered_value newRetranslKeysTbl = toml::parse_str<toml::ordered_type_config>(retranslKeyEdit->toPlainText().toStdString());
				auto& newRetranslKeysArr = newRetranslKeysTbl["retranslKeys"];
				if (newRetranslKeysArr.is_array()) {
					insertToml(_projectConfig, "problemAnalyze.retranslKeys", newRetranslKeysArr.as_array());
				}
				else {
					insertToml(_projectConfig, "problemAnalyze.retranslKeys", toml::array{});
				}
			}
			catch (...) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("解析错误"), tr("retranslKeys不符合 toml 规范"), 3000);
			}

			try {
				toml::ordered_value newCompareObjTbl = toml::parse_str<toml::ordered_type_config>(compareObjEdit->toPlainText().toStdString());
				auto& newCompareObjArr = newCompareObjTbl["overwriteCompareObj"];
				if (newCompareObjArr.is_array()) {
					newCompareObjArr.as_array_fmt().fmt = toml::array_format::multiline;
					insertToml(_projectConfig, "problemAnalyze.overwriteCompareObj", newCompareObjArr.as_array());
				}
				else {
					insertToml(_projectConfig, "problemAnalyze.overwriteCompareObj", toml::array{});
				}
			}
			catch (...) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("解析错误"), tr("overwriteCompareObj不符合 toml 规范"), 3000);
			}
		};

	addCentralWidget(mainWidget, true, true, 0);
}