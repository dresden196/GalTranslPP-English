#include "PASettingsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QButtonGroup>
#include <QFileDialog>

#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaScrollPageArea.h"
#include "ElaToolTip.h"
#include "ElaMessageBar.h"
#include "ElaPushButton.h"
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
		"残留日文","引入拉丁字母","引入韩文", "引入繁体字","语言不通", };
	QStringList problemListToShow = { tr("词频过高"), tr("标点错漏"), tr("丢失换行"), tr("多加换行"), tr("比原文长"), tr("比原文长严格"),
		tr("字典未使用"), tr("残留日文"), tr("引入拉丁字母"), tr("引入韩文"), tr("引入繁体字"), tr("语言不通"),};

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
		if (problemListToShow[0][0] != 'H') {
			button->setFixedWidth(120);
		}
		else {
			button->setFixedWidth(180);
		}
		if (problemListSet.contains(problemList[i].toStdString())) {
			button->setIsToggled(true);
		}
		problemListLayout->addWidget(button);
		problemListButtons.append(button);
	}
	mainLayout->addLayout(problemListLayout);
	mainLayout->addSpacing(20);

	// 规定标点错漏要查哪些标点
	std::string punctuationSet = toml::find_or(_projectConfig, "problemAnalyze", "punctSet", "");
	QString punctuationSetStr = QString::fromStdString(punctuationSet);
	ElaScrollPageArea* punctuationListArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* punctuationListLayout = new QHBoxLayout(punctuationListArea);
	ElaText* punctuationListTitle = new ElaText(tr("标点查错"), punctuationListArea);
	punctuationListTitle->setWordWrap(false);
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
	double languageProbability = toml::find_or(_projectConfig, "problemAnalyze", "langProbability", 0.94);
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

	auto createPAPlainTextEditAreaFunc = 
		[=](const std::string& configKey, const QString& title, std::optional<int> minHeight = std::nullopt)
		-> std::function<void()>
		{
			toml::ordered_value retranslKeysArr = toml::array{};
			auto& retranslKeys = _projectConfig["problemAnalyze"][configKey];
			if (retranslKeys.is_array()) {
				retranslKeysArr = retranslKeys;
			}
			retranslKeysArr.comments().clear();
			ElaText* retranslKeyHelperText = new ElaText(title, mainWidget);
			ElaToolTip* retranslKeyHelperTip = new ElaToolTip(retranslKeyHelperText);
			retranslKeyHelperTip->setToolTip(tr("点击下方『语法示例』按钮以获取具体语法规则及作用"));
			retranslKeyHelperText->setTextPixelSize(18);
			retranslKeyHelperText->setWordWrap(false);
			mainLayout->addWidget(retranslKeyHelperText);
			ElaPlainTextEdit* retranslKeyEdit = new ElaPlainTextEdit(mainWidget);
			if (minHeight) {
				retranslKeyEdit->setMinimumHeight(*minHeight);
			}
			QFont font = retranslKeyEdit->font();
			font.setPixelSize(14);
			retranslKeyEdit->setFont(font);
			retranslKeyEdit->setPlainText(QString::fromStdString(toml::format(toml::ordered_value{ toml::ordered_table{{ configKey, retranslKeysArr }} })));
			retranslKeyEdit->moveCursor(QTextCursor::Start);
			mainLayout->addWidget(retranslKeyEdit);

			std::function<void()> saveFunc = [=]()
				{
					try {
						toml::ordered_value newRetranslKeysTbl = toml::parse_str<toml::ordered_type_config>(retranslKeyEdit->toPlainText().toStdString());
						auto& newRetranslKeysArr = newRetranslKeysTbl[configKey];
						if (newRetranslKeysArr.is_array()) {
							if (configKey == "retranslKeys") {
								for (auto& rkey : newRetranslKeysArr.as_array()) {
									if (!rkey.is_string()) {
										continue;
									}
									int index = problemListToShow.indexOf(rkey.as_string());
									if (index < 0) {
										continue;
									}
									rkey = problemList[index].toStdString();
								}
							}
							insertToml(_projectConfig, "problemAnalyze." + configKey, newRetranslKeysArr);
						}
						else {
							insertToml(_projectConfig, "problemAnalyze." + configKey, toml::array{});
						}
					}
					catch (...) {
						ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("解析错误"), QString::fromStdString(configKey) + tr("不符合 toml 规范"), 3000);
					}
				};
			return saveFunc;
		};

	// 正则表达式列表，重翻正则在缓存的 orig_text 或 某条 problem 中能 search 通过的句子。
	auto retranslKeysSaveFunc = createPAPlainTextEditAreaFunc("retranslKeys", tr("重翻关键字设定"), 250);
	mainLayout->addSpacing(20);

	// 正则表达式列表，如果一条 problem 能被以下正则 search 通过，则不加入 problems 列表
	auto skipProblemsSaveFunc = createPAPlainTextEditAreaFunc("skipProblems", tr("跳过问题关键字设定"), 310);
	mainLayout->addSpacing(20);

	// 问题的比较对象和被比较对象(不写则默认为orig_text和transPreview)
	auto overwriteCompareObjSaveFunc = createPAPlainTextEditAreaFunc("overwriteCompareObj", tr("问题比较对象设定"), 280);

	QWidget* illusButtonWidget = new QWidget(mainWidget);
	QHBoxLayout* illusButtonLayout = new QHBoxLayout(illusButtonWidget);
	illusButtonLayout->addStretch();
	ElaPushButton* illusButton = new ElaPushButton(tr("语法示例"), illusButtonWidget);
	ElaToolTip* illusButtonTip = new ElaToolTip(illusButton);
	illusButtonTip->setToolTip(tr("查看 重翻关键字/跳过问题关键字/问题比较对象 设定的语法示例"));
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

			retranslKeysSaveFunc();
			skipProblemsSaveFunc();
			overwriteCompareObjSaveFunc();
		};

	addCentralWidget(mainWidget, true, true, 0);
}