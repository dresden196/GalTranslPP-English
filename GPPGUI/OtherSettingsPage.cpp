#include "OtherSettingsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QButtonGroup>
#include <QFileDialog>

#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaScrollPageArea.h"
#include "ElaPushButton.h"
#include "ElaMessageBar.h"
#include "ElaToolTip.h"
#include "ElaContentDialog.h"
#include "ElaInputDialog.h"

import Tool;

OtherSettingsPage::OtherSettingsPage(QWidget* mainWindow, fs::path& projectDir, toml::ordered_value& globalConfig, toml::ordered_value& projectConfig, QWidget* parent) :
	BasePage(parent), _projectConfig(projectConfig), _globalConfig(globalConfig), _projectDir(projectDir), _mainWindow(mainWindow)
{
	setWindowTitle(tr("其它设置"));
	setTitleVisible(false);

	_setupUI();
}

OtherSettingsPage::~OtherSettingsPage()
{

}

void OtherSettingsPage::_setupUI()
{
	QWidget* mainWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(5);

	// 项目路径
	ElaScrollPageArea* pathArea = new ElaScrollPageArea(mainWidget);
	pathArea->setFixedHeight(100);
	QHBoxLayout* pathLayout = new QHBoxLayout(pathArea);
	ElaText* pathLabel = new ElaText(pathArea);
	pathLabel->setText(tr("项目路径"));
	pathLabel->setTextPixelSize(16);
	pathLayout->addWidget(pathLabel);
	pathLayout->addStretch();
	ElaLineEdit* pathEdit = new ElaLineEdit(pathArea);
	pathEdit->setReadOnly(true);
	pathEdit->setText(QString(_projectDir.wstring()));
	pathEdit->setFixedWidth(550);
	pathLayout->addWidget(pathEdit);
	ElaPushButton* openButton = new ElaPushButton(pathArea);
	openButton->setText(tr("打开文件夹"));
	connect(openButton, &ElaPushButton::clicked, this, [=]()
		{
			QUrl dirUrl = QUrl::fromLocalFile(QString(_projectDir.wstring()));
			QDesktopServices::openUrl(dirUrl);
		});
	pathLayout->addWidget(openButton);
	ElaPushButton* moveButton = new ElaPushButton(pathArea);
	moveButton->setText(tr("移动项目"));
	connect(moveButton, &ElaPushButton::clicked, this, [=]()
		{
			if (toml::get_or(_projectConfig["GUIConfig"]["isRunning"], true)) {
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("移动失败"), tr("项目仍在运行中，无法移动"), 3000);
				return;
			}

			QString newProjectParentPath = QFileDialog::getExistingDirectory(this, tr("请选择要移动到的文件夹"), QString::fromStdString(toml::find_or(_globalConfig, "lastProjectPath", "./Projects")));
			if (newProjectParentPath.isEmpty()) {
				return;
			}
			insertToml(_globalConfig, "lastProjectPath", newProjectParentPath.toStdString());
			fs::path newProjectPath = fs::path(newProjectParentPath.toStdWString()) / _projectDir.filename();
			if (fs::exists(newProjectPath)) {
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("移动失败"), tr("目录下已有同名文件夹"), 3000);
				return;
			}

			try {
				fs::rename(_projectDir, newProjectPath);
			}
			catch (const fs::filesystem_error& e) {
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("移动失败"), QString(e.what()), 3000);
				return;
			}
			_projectDir = newProjectPath;
			pathEdit->setText(QString(_projectDir.wstring()));
			ElaMessageBar::success(ElaMessageBarType::TopRight, tr("移动成功"), QString(_projectDir.filename().wstring()) + tr(" 项目已移动到新文件夹"), 3000);
		});
	pathLayout->addWidget(moveButton);
	mainLayout->addWidget(pathArea);


	// 项目更名
	ElaScrollPageArea* renameArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* renameLayout = new QHBoxLayout(renameArea);
	ElaText* renameLabel = new ElaText(renameArea);
	renameLabel->setWordWrap(false);
	renameLabel->setText(tr("项目更名"));
	renameLabel->setTextPixelSize(16);
	renameLayout->addWidget(renameLabel);
	renameLayout->addStretch();
	ElaPushButton* renameButton = new ElaPushButton(renameArea);
	renameButton->setText(tr("更名"));
	connect(renameButton, &ElaPushButton::clicked, this, [=]()
		{
			if (toml::get_or(_projectConfig["GUIConfig"]["isRunning"], true)) {
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("更名失败"), tr("项目仍在运行中，无法更名"), 3000);
				return;
			}

			bool ok;
			QString newProjectName;
			ElaInputDialog inputDialog(_mainWindow, tr("请输入新的项目名称"), tr("新的项目名"), newProjectName, &ok);
			inputDialog.exec();
			if (!ok) {
				return;
			}
			if(newProjectName.isEmpty() || newProjectName.contains("\\") || newProjectName.contains("/")){
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("更名失败"), tr("项目名不能为空且不能包含斜杠"), 3000);
				return;
			}

			fs::path newProjectPath = _projectDir.parent_path() / newProjectName.toStdWString();
			if (fs::exists(newProjectPath)) {
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("更名失败"), tr("目录下已有同名文件或文件夹"), 3000);
				return;
			}

			try {
				fs::rename(_projectDir, newProjectPath);
			}
			catch (const fs::filesystem_error& e) {
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("更名失败"), QString(e.what()), 3000);
				return;
			}
			_projectDir = newProjectPath;
			pathEdit->setText(QString(_projectDir.wstring()));
			Q_EMIT changeProjectNameSignal(newProjectName);
			ElaMessageBar::success(ElaMessageBarType::TopRight, tr("更名成功"), tr("项目已更名为 ") + newProjectName, 3000);
		});
	renameLayout->addWidget(renameButton);
	mainLayout->addWidget(renameArea);


	// 保存配置
	ElaScrollPageArea* saveArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* saveLayout = new QHBoxLayout(saveArea);
	ElaText* saveLabel = new ElaText(saveArea);
	ElaToolTip* saveTip = new ElaToolTip(saveLabel);
	saveTip->setToolTip(tr("开始翻译或关闭程序时会自动保存所有项目的配置，一般无需手动保存。"));
	saveLabel->setText(tr("保存项目配置"));
	saveLabel->setWordWrap(false);
	saveLabel->setTextPixelSize(16);
	saveLayout->addWidget(saveLabel);
	saveLayout->addStretch();
	ElaPushButton* saveButton = new ElaPushButton(saveArea);
	saveButton->setText(tr("保存"));
	connect(saveButton, &ElaPushButton::clicked, this, [=]()
		{
			Q_EMIT saveConfigSignal();
			ElaMessageBar::success(ElaMessageBarType::TopRight, tr("保存成功"), tr("项目 ") + QString(_projectDir.filename().wstring()) + tr(" 的配置信息已保存"), 3000);
		});
	saveLayout->addWidget(saveButton);
	mainLayout->addWidget(saveArea);


	// 刷新项目配置
	ElaScrollPageArea* refreshArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* refreshLayout = new QHBoxLayout(refreshArea);
	ElaText* refreshLabel = new ElaText(tr("刷新项目配置"), refreshArea);
	refreshLabel->setTextPixelSize(16);
	refreshLabel->setWordWrap(false);
	ElaToolTip* refreshTip = new ElaToolTip(refreshLabel);
	refreshTip->setToolTip(tr("将刷新现有配置和字典，谨慎使用。"));
	refreshLayout->addWidget(refreshLabel);
	refreshLayout->addStretch();
	ElaPushButton* refreshButton = new ElaPushButton(refreshArea);
	refreshButton->setText(tr("刷新"));
	connect(refreshButton, &ElaPushButton::clicked, this, [=]()
		{
			ElaContentDialog helpDialog(_mainWindow);
			helpDialog.setLeftButtonText(tr("否"));
			helpDialog.setMiddleButtonText(tr("思考人生"));
			helpDialog.setRightButtonText(tr("是"));
			QWidget* widget = new QWidget(&helpDialog);
			QVBoxLayout* layout = new QVBoxLayout(widget);
			ElaText* confirmText = new ElaText(tr("你确定要刷新项目配置吗？"), 18, widget);
			confirmText->setWordWrap(false);
			layout->addWidget(confirmText);
			layout->addWidget(new ElaText(tr("GUI中未保存的数据将会被覆盖！"), 16, widget));
			helpDialog.setCentralWidget(widget);
			connect(&helpDialog, &ElaContentDialog::rightButtonClicked, this, [=]()
				{
					Q_EMIT refreshProjectConfigSignal();
				});
			helpDialog.exec();
		});
	refreshLayout->addWidget(refreshButton);
	mainLayout->addWidget(refreshArea);


	// 删除翻译缓存
	ElaScrollPageArea* cacheArea = new ElaScrollPageArea(mainWidget);
	QHBoxLayout* cacheLayout = new QHBoxLayout(cacheArea);
	ElaText* cacheLabel = new ElaText(cacheArea);
	cacheLabel->setText(tr("删除翻译缓存"));
	cacheLabel->setWordWrap(false);
	cacheLabel->setTextPixelSize(16);
	cacheLayout->addWidget(cacheLabel);
	cacheLayout->addStretch();
	ElaPushButton* cacheButton = new ElaPushButton(cacheArea);
	cacheButton->setText(tr("删除"));
	connect(cacheButton, &ElaPushButton::clicked, this, [=]()
		{
			if (toml::get_or(_projectConfig["GUIConfig"]["isRunning"], true)) {
				ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("删除失败"), tr("项目仍在运行中，无法删除缓存"), 3000);
				return;
			}

			ElaContentDialog helpDialog(_mainWindow);
			helpDialog.setLeftButtonText(tr("否"));
			helpDialog.setMiddleButtonText(tr("思考人生"));
			helpDialog.setRightButtonText(tr("是"));
			QWidget* widget = new QWidget(&helpDialog);
			QVBoxLayout* layout = new QVBoxLayout(widget);
			ElaText* confirmText = new ElaText(tr("你确定要删除项目翻译缓存吗？"), 18, widget);
			confirmText->setWordWrap(false);
			layout->addWidget(confirmText);
			layout->addWidget(new ElaText(tr("再次翻译将会重新从头开始！"), 16, widget));
			helpDialog.setCentralWidget(widget);
			connect(&helpDialog, &ElaContentDialog::rightButtonClicked, this, [=]()
				{
					try {
						fs::remove_all(_projectDir / L"transl_cache");
					}
					catch (const fs::filesystem_error& e) {
						ElaMessageBar::warning(ElaMessageBarType::TopRight, tr("删除失败"), QString(e.what()), 3000);
						return;
					}
					ElaMessageBar::success(ElaMessageBarType::TopRight, tr("删除成功"), tr("项目 ") + QString(_projectDir.filename().wstring()) + tr(" 的翻译缓存已删除"), 3000);

				});
			helpDialog.exec();
		});
	cacheLayout->addWidget(cacheButton);
	mainLayout->addWidget(cacheArea);
	
	mainLayout->addStretch();
	addCentralWidget(mainWidget, true, true, 0);
}