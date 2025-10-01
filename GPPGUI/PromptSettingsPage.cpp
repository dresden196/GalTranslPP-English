#include "PromptSettingsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStackedWidget>

#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaScrollPageArea.h"
#include "ElaToolTip.h"
#include "ElaTabWidget.h"
#include "ElaTableView.h"
#include "ElaPushButton.h"
#include "ElaMessageBar.h"
#include "ElaToggleSwitch.h"
#include "ElaPlainTextEdit.h"

import Tool;

PromptSettingsPage::PromptSettingsPage(fs::path& projectDir, toml::value& projectConfig, QWidget* parent) :
	BasePage(parent), _projectConfig(projectConfig), _projectDir(projectDir)
{
	setWindowTitle(tr("项目提示词设置"));
	setTitleVisible(false);

	if (fs::exists(_projectDir / L"Prompt.toml")) {
		try {
			_promptConfig = toml::parse(_projectDir / L"Prompt.toml");
		}
		catch (...) {
			ElaMessageBar::error(ElaMessageBarType::TopRight, tr("解析失败"), tr("项目 ") +
				QString(_projectDir.filename().wstring()) + tr(" 的提示词配置文件不符合标准。"), 3000);
		}
	}
	
	_setupUI();
}

PromptSettingsPage::~PromptSettingsPage()
{

}


void PromptSettingsPage::_setupUI()
{
	QWidget* mainWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
	mainLayout->setContentsMargins(0, 0, 0, 0);

	ElaTabWidget* tabWidget = new ElaTabWidget(mainWidget);
	tabWidget->setTabsClosable(false);
	tabWidget->setIsTabTransparent(true);


	auto createPromptWidgetFunc =
		[=](const QString& promptName, const std::string& userPromptKey, const std::string& systemPromptKey) -> std::function<void()>
		{
			QWidget* forgalJsonWidget = new QWidget(mainWidget);
			QVBoxLayout* forgalJsonLayout = new QVBoxLayout(forgalJsonWidget);
			QWidget* forgalJsonButtonWidget = new QWidget(mainWidget);
			QHBoxLayout* forgalJsonButtonLayout = new QHBoxLayout(forgalJsonButtonWidget);
			ElaPushButton* forgalJsonUserModeButtom = new ElaPushButton(forgalJsonButtonWidget);
			forgalJsonUserModeButtom->setText(tr("用户提示词"));
			forgalJsonUserModeButtom->setEnabled(false);
			ElaPushButton* forgalJsonSystemModeButtom = new ElaPushButton(forgalJsonButtonWidget);
			forgalJsonSystemModeButtom->setText(tr("系统提示词"));
			forgalJsonSystemModeButtom->setEnabled(true);
			forgalJsonButtonLayout->addWidget(forgalJsonUserModeButtom);
			forgalJsonButtonLayout->addWidget(forgalJsonSystemModeButtom);
			forgalJsonButtonLayout->addStretch();
			forgalJsonLayout->addWidget(forgalJsonButtonWidget, 0, Qt::AlignTop);

			QStackedWidget* forgalJsonStackedWidget = new QStackedWidget(forgalJsonWidget);
			// 用户提示词
			ElaPlainTextEdit* forgalJsonUserModeEdit = new ElaPlainTextEdit(forgalJsonStackedWidget);
			QFont plainTextFont = forgalJsonUserModeEdit->font();
			plainTextFont.setPixelSize(15);
			forgalJsonUserModeEdit->setFont(plainTextFont);
			forgalJsonUserModeEdit->setPlainText(
				QString::fromStdString(toml::get_or(_promptConfig[userPromptKey], std::string{}))
			);
			forgalJsonStackedWidget->addWidget(forgalJsonUserModeEdit);
			// 系统提示词
			ElaPlainTextEdit* forgalJsonSystemModeEdit = new ElaPlainTextEdit(forgalJsonStackedWidget);
			forgalJsonSystemModeEdit->setFont(plainTextFont);
			forgalJsonSystemModeEdit->setPlainText(
				QString::fromStdString(toml::get_or(_promptConfig[systemPromptKey], std::string{}))
			);
			forgalJsonStackedWidget->addWidget(forgalJsonSystemModeEdit);
			forgalJsonStackedWidget->setCurrentIndex(0);
			forgalJsonLayout->addWidget(forgalJsonStackedWidget, 1);
			connect(forgalJsonUserModeButtom, &ElaPushButton::clicked, this, [=]()
				{
					forgalJsonStackedWidget->setCurrentIndex(0);
					forgalJsonUserModeButtom->setEnabled(false);
					forgalJsonSystemModeButtom->setEnabled(true);
				});
			connect(forgalJsonSystemModeButtom, &ElaPushButton::clicked, this, [=]()
				{
					forgalJsonStackedWidget->setCurrentIndex(1);
					forgalJsonUserModeButtom->setEnabled(true);
					forgalJsonSystemModeButtom->setEnabled(false);
				});
			tabWidget->addTab(forgalJsonWidget, promptName);

			auto result = [=]()
				{
					insertToml(_promptConfig, userPromptKey, forgalJsonUserModeEdit->toPlainText().toStdString());
					insertToml(_promptConfig, systemPromptKey, forgalJsonSystemModeEdit->toPlainText().toStdString());
				};
			return result;
		};


	auto forgalJsonApplyFunc = createPromptWidgetFunc(tr("ForGalJson"), "FORGALJSON_TRANS_PROMPT_EN", "FORGALJSON_SYSTEM");
	auto forgalTsvApplyFunc = createPromptWidgetFunc(tr("ForGalTsv"), "FORGALTSV_TRANS_PROMPT_EN", "FORGALTSV_SYSTEM");
	auto forNovelTsvApplyFunc = createPromptWidgetFunc(tr("ForNovelTsv"), "FORNOVELTSV_TRANS_PROMPT_EN", "FORNOVELTSV_SYSTEM");
	auto deepSeekApplyFunc = createPromptWidgetFunc(tr("DeepSeek"), "DEEPSEEKJSON_TRANS_PROMPT", "DEEPSEEKJSON_SYSTEM_PROMPT");
	auto sakuraApplyFunc = createPromptWidgetFunc(tr("Sakura"), "SAKURA_TRANS_PROMPT_EN", "SAKURA_SYSTEM");
	auto gendicApplyFunc = createPromptWidgetFunc(tr("GenDict"), "GENDIC_PROMPT", "GENDIC_SYSTEM");

	_applyFunc = [=]()
		{
			forgalJsonApplyFunc();
			forgalTsvApplyFunc();
			forNovelTsvApplyFunc();
			deepSeekApplyFunc();
			sakuraApplyFunc();
			gendicApplyFunc();
			std::ofstream ofs(_projectDir / L"Prompt.toml");
			ofs << _promptConfig;
			ofs.close();
		};

	mainLayout->addWidget(tabWidget);
	addCentralWidget(mainWidget, true, true, 0);
}
