#include "CommonNormalDictPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <nlohmann/json.hpp>

#include "ElaText.h"
#include "ElaScrollPageArea.h"
#include "ElaToolTip.h"
#include "ElaIconButton.h"
#include "ElaTableView.h"
#include "ElaPushButton.h"
#include "ElaMessageBar.h"
#include "ElaToggleButton.h"
#include "ElaPlainTextEdit.h"
#include "ElaTabWidget.h"
#include "ElaInputDialog.h"
#include "ReadDicts.h"

import Tool;
namespace fs = std::filesystem;
using json = nlohmann::json;

CommonNormalDictPage::CommonNormalDictPage(const std::string& mode, toml::value& globalConfig, QWidget* parent) :
	BasePage(parent), _globalConfig(globalConfig), _mainWindow(parent)
{
	setWindowTitle(tr("默认译前字典设置"));
	setTitleVisible(false);
	setContentsMargins(5, 5, 5, 5);

	if (mode == "pre") {
		_modeConfig = "commonPreDicts";
		_modePath = "pre";
	}
	else if (mode == "post") {
		_modeConfig = "commonPostDicts";
		_modePath = "post";
	}
	else {
		QMessageBox::critical(parent, tr("错误"), tr("未知通用字典模式"), QMessageBox::Ok);
		exit(1);
	}

	_setupUI();
}

CommonNormalDictPage::~CommonNormalDictPage()
{

}

void CommonNormalDictPage::_setupUI()
{
	QWidget* mainWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);

	QWidget* mainButtonWidget = new QWidget(mainWidget);
	QHBoxLayout* mainButtonLayout = new QHBoxLayout(mainButtonWidget);
	ElaText* dictNameLabel = new ElaText(mainButtonWidget);
	dictNameLabel->setTextPixelSize(18);
	QString dictNameText;
	if (_modePath == "pre") {
		dictNameText = tr("通用译前字典");
	}
	else if (_modePath == "post") {
		dictNameText = tr("通用译后字典");
	}
	dictNameLabel->setText(dictNameText);
	ElaPushButton* importButton = new ElaPushButton(mainButtonWidget);
	importButton->setText(tr("导入字典页"));
	ElaPushButton* addNewTabButton = new ElaPushButton(mainButtonWidget);
	addNewTabButton->setText(tr("添加新字典页"));
	ElaPushButton* removeTabButton = new ElaPushButton(mainButtonWidget);
	removeTabButton->setText(tr("移除当前页"));
	mainButtonLayout->addSpacing(10);
	mainButtonLayout->addWidget(dictNameLabel);
	mainButtonLayout->addStretch();
	mainButtonLayout->addWidget(importButton);
	mainButtonLayout->addWidget(addNewTabButton);
	mainButtonLayout->addWidget(removeTabButton);
	mainLayout->addWidget(mainButtonWidget, 0, Qt::AlignTop);

	ElaTabWidget* tabWidget = new ElaTabWidget(mainWidget);
	tabWidget->setTabsClosable(false);
	tabWidget->setIsTabTransparent(true);

	auto createNormalTab = [=](const fs::path& orgDictPath) -> QWidget*
		{
			fs::path dictPath = L"BaseConfig/Dict/" + ascii2Wide(_modePath) + L"/" + fs::path(orgDictPath.filename()).replace_extension(L".toml").wstring();
			std::string dictName = wide2Ascii(orgDictPath.stem().wstring());
			NormalTabEntry normalTabEntry;

			QWidget* pageMainWidget = new QWidget(tabWidget);
			QVBoxLayout* pageMainLayout = new QVBoxLayout(pageMainWidget);

			QWidget* pageButtonWidget = new QWidget(pageMainWidget);
			QHBoxLayout* pageButtonLayout = new QHBoxLayout(pageButtonWidget);
			ElaPushButton* plainTextModeButton = new ElaPushButton(mainButtonWidget);
			plainTextModeButton->setText(tr("纯文本模式"));
			ElaPushButton* tableModeButton = new ElaPushButton(mainButtonWidget);
			tableModeButton->setText(tr("表模式"));
			ElaToggleButton* defaultOnButton = new ElaToggleButton(mainButtonWidget);
			defaultOnButton->setText(tr("默认启用"));
			ElaIconButton* saveAllButton = new ElaIconButton(ElaIconType::CheckDouble, mainButtonWidget);
			saveAllButton->setFixedWidth(30);
			ElaToolTip* saveAllButtonToolTip = new ElaToolTip(saveAllButton);
			saveAllButtonToolTip->setToolTip(tr("保存所有页"));
			ElaIconButton* saveButton = new ElaIconButton(ElaIconType::Check, mainButtonWidget);
			saveButton->setFixedWidth(30);
			ElaToolTip* saveButtonToolTip = new ElaToolTip(saveButton);
			saveButtonToolTip->setToolTip(tr("保存当前页"));
			ElaIconButton* withdrawButton = new ElaIconButton(ElaIconType::ArrowLeft, mainButtonWidget);
			withdrawButton->setFixedWidth(30);
			ElaToolTip* withdrawButtonToolTip = new ElaToolTip(withdrawButton);
			withdrawButtonToolTip->setToolTip(tr("撤回删除行"));
			withdrawButton->setEnabled(false);
			ElaIconButton* refreshButton = new ElaIconButton(ElaIconType::ArrowRotateRight, mainButtonWidget);
			refreshButton->setFixedWidth(30);
			ElaToolTip* refreshButtonToolTip = new ElaToolTip(refreshButton);
			refreshButtonToolTip->setToolTip(tr("刷新当前页"));
			ElaIconButton* addDictButton = new ElaIconButton(ElaIconType::Plus, mainButtonWidget);
			addDictButton->setFixedWidth(30);
			ElaToolTip* addDictButtonToolTip = new ElaToolTip(addDictButton);
			addDictButtonToolTip->setToolTip(tr("添加词条"));
			ElaIconButton* removeDictButton = new ElaIconButton(ElaIconType::Minus, mainButtonWidget);
			removeDictButton->setFixedWidth(30);
			ElaToolTip* removeDictButtonToolTip = new ElaToolTip(removeDictButton);
			removeDictButtonToolTip->setToolTip(tr("删除词条"));
			pageButtonLayout->addWidget(plainTextModeButton);
			pageButtonLayout->addWidget(tableModeButton);
			pageButtonLayout->addWidget(defaultOnButton);
			pageButtonLayout->addStretch();
			pageButtonLayout->addWidget(saveAllButton);
			pageButtonLayout->addWidget(saveButton);
			pageButtonLayout->addWidget(withdrawButton);
			pageButtonLayout->addWidget(refreshButton);
			pageButtonLayout->addWidget(addDictButton);
			pageButtonLayout->addWidget(removeDictButton);
			pageMainLayout->addWidget(pageButtonWidget, 0, Qt::AlignTop);

			QStackedWidget* stackedWidget = new QStackedWidget(tabWidget);
			ElaPlainTextEdit* plainTextEdit = new ElaPlainTextEdit(stackedWidget);
			QFont plainTextFont = plainTextEdit->font();
			plainTextFont.setPixelSize(15);
			plainTextEdit->setFont(plainTextFont);
			plainTextEdit->setPlainText(ReadDicts::readDictsStr(orgDictPath));
			stackedWidget->addWidget(plainTextEdit);
			ElaTableView* tableView = new ElaTableView(stackedWidget);
			QFont tableHeaderFont = tableView->horizontalHeader()->font();
			tableHeaderFont.setPixelSize(16);
			tableView->horizontalHeader()->setFont(tableHeaderFont);
			tableView->verticalHeader()->setHidden(true);
			tableView->setAlternatingRowColors(true);
			tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
			NormalDictModel* model = new NormalDictModel(tableView);
			QList<NormalDictEntry> normalData = ReadDicts::readNormalDicts(orgDictPath);
			model->loadData(normalData);
			tableView->setModel(model);
			stackedWidget->addWidget(tableView);
			stackedWidget->setCurrentIndex(toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["openMode"], 1));
			tableView->setColumnWidth(0, toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["columnWidth"]["0"], 200));
			tableView->setColumnWidth(1, toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["columnWidth"]["1"], 150));
			tableView->setColumnWidth(2, toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["columnWidth"]["2"], 100));
			tableView->setColumnWidth(3, toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["columnWidth"]["3"], 172));
			tableView->setColumnWidth(4, toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["columnWidth"]["4"], 75));
			tableView->setColumnWidth(5, toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["columnWidth"]["5"], 60));
			pageMainLayout->addWidget(stackedWidget, 1);

			plainTextModeButton->setEnabled(stackedWidget->currentIndex() != 0);
			tableModeButton->setEnabled(stackedWidget->currentIndex() != 1);
			addDictButton->setEnabled(stackedWidget->currentIndex() == 1);
			removeDictButton->setEnabled(stackedWidget->currentIndex() == 1);
			defaultOnButton->setIsToggled(toml::get_or(_globalConfig[_modeConfig]["spec"][dictName]["defaultOn"], true));
			insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".defaultOn", defaultOnButton->getIsToggled());

			connect(plainTextModeButton, &ElaPushButton::clicked, this, [=]()
				{
					stackedWidget->setCurrentIndex(0);
					plainTextModeButton->setEnabled(false);
					tableModeButton->setEnabled(true);
					addDictButton->setEnabled(false);
					removeDictButton->setEnabled(false);
					withdrawButton->setEnabled(false);
				});

			connect(tableModeButton, &ElaPushButton::clicked, this, [=]()
				{
					stackedWidget->setCurrentIndex(1);
					plainTextModeButton->setEnabled(true);
					tableModeButton->setEnabled(false);
					addDictButton->setEnabled(true);
					removeDictButton->setEnabled(true);
					withdrawButton->setEnabled(!normalTabEntry.withdrawList->empty());
				});

			connect(defaultOnButton, &ElaToggleButton::toggled, this, [=](bool checked)
				{
					insertToml(_globalConfig, _modeConfig + ".spec." + dictName
						+ ".defaultOn", checked);
				});

			connect(saveAllButton, &ElaPushButton::clicked, this, [=]()
				{
					this->apply2Config();
					ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("保存成功"), tr("所有默认字典配置均已保存"), 3000);
				});

			auto saveFunc = [=](bool forceSaveInTableModeToInit) // param 导入时先强制保存一下来给纯文本模式作初始化
				{
					std::ofstream ofs(dictPath);
					if (!ofs.is_open()) {
						ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("保存失败"), tr("无法打开字典: ") +
							QString(dictPath.wstring()), 3000);
						return;
					}

					if (stackedWidget->currentIndex() == 0 && !forceSaveInTableModeToInit) {
						ofs << plainTextEdit->toPlainText().toStdString();
						ofs.close();
						QList<NormalDictEntry> newDictEntries = ReadDicts::readNormalDicts(dictPath);
						model->loadData(newDictEntries);
					}
					else if (stackedWidget->currentIndex() == 1 || forceSaveInTableModeToInit) {
						QList<NormalDictEntry> dictEntries = model->getEntries();
						toml::value dictArr= toml::array{};
						for (const auto& entry : dictEntries) {
							toml::ordered_table dictTable;
							dictTable.insert({ "org", entry.original.toStdString() });
							dictTable.insert({ "rep", entry.translation.toStdString() });
							dictTable.insert({ "conditionTarget", entry.conditionTar.toStdString() });
							dictTable.insert({ "conditionReg", entry.conditionReg.toStdString() });
							dictTable.insert({ "isReg", entry.isReg });
							dictTable.insert({ "priority", entry.priority });
							dictArr.push_back(dictTable);
						}
						ofs << toml::format(toml::value{ toml::table{{"normalDict", dictArr}} });
						ofs.close();
						plainTextEdit->setPlainText(ReadDicts::readDictsStr(dictPath));
					}

					auto& newDictNames = _globalConfig[_modeConfig]["dictNames"];
					if (!newDictNames.is_array()) {
						insertToml(_globalConfig, _modeConfig + ".dictNames", toml::array{ dictName });
					}
					else {
						if (
							!std::ranges::any_of(newDictNames.as_array(), [=](const toml::value& name)
							{
								return name.is_string() && name.as_string() == dictName;
							})
							) {
							newDictNames.push_back(dictName);
						}
					}
					if (!forceSaveInTableModeToInit) {
						Q_EMIT commonDictsChanged();
						ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("保存成功"), tr("字典 ") +
							QString::fromStdString(dictName) + tr(" 已保存"), 3000);
					}
				};
			connect(saveButton, &ElaPushButton::clicked, this, [=]()
				{
					saveFunc(false);
				});

			connect(addDictButton, &ElaPushButton::clicked, this, [=]()
				{
					QModelIndexList selectedRows = tableView->selectionModel()->selectedRows();
					if (selectedRows.isEmpty()) {
						model->insertRow(model->rowCount());
					}
					else {
						model->insertRow(selectedRows.first().row());
					}
				});

			connect(removeDictButton, &ElaPushButton::clicked, this, [=]()
				{
					QModelIndexList selectedRows = tableView->selectionModel()->selectedRows();
					const QList<NormalDictEntry>& entries = model->getEntries();
					std::ranges::sort(selectedRows, [](const QModelIndex& a, const QModelIndex& b)
						{
							return a.row() > b.row();
						});
					for (const auto& index : selectedRows) {
						if (normalTabEntry.withdrawList->size() > 100) {
							normalTabEntry.withdrawList->pop_front();
						}
						normalTabEntry.withdrawList->push_back(entries[index.row()]);
						model->removeRow(index.row());
					}
					if (!selectedRows.isEmpty()) {
						withdrawButton->setEnabled(true);
					}
				});
			connect(withdrawButton, &ElaPushButton::clicked, this, [=]()
				{
					if (normalTabEntry.withdrawList->empty()) {
						return;
					}
					NormalDictEntry entry = normalTabEntry.withdrawList->front();
					normalTabEntry.withdrawList->pop_front();
					model->insertRow(0, entry);
					if (normalTabEntry.withdrawList->empty()) {
						withdrawButton->setEnabled(false);
					}
				});

			connect(refreshButton, &ElaPushButton::clicked, this, [=]()
				{
					plainTextEdit->setPlainText(ReadDicts::readDictsStr(dictPath));
					model->loadData(ReadDicts::readNormalDicts(dictPath));
					ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("刷新成功"), tr("字典 ") +
						QString::fromStdString(dictName) + tr(" 已刷新"), 3000);
				});

			normalTabEntry.pageMainWidget = pageMainWidget;
			normalTabEntry.stackedWidget = stackedWidget;
			normalTabEntry.plainTextEdit = plainTextEdit;
			normalTabEntry.tableView = tableView;
			normalTabEntry.dictModel = model;
			normalTabEntry.dictPath = dictPath;
			_normalTabEntries.push_back(normalTabEntry);

			if (!fs::exists(dictPath)) {
				saveFunc(true);
			}
			return pageMainWidget;
		};

	auto& commonNormalDicts = _globalConfig[_modeConfig]["dictNames"];
	if (commonNormalDicts.is_array()) {
		toml::array newDictNames;
		for (const auto& dictName : commonNormalDicts.as_array()) {
			fs::path dictPath = L"BaseConfig/Dict/" + ascii2Wide(_modePath) + L"/" + ascii2Wide(dictName.as_string()) + L".toml";
			if (!fs::exists(dictPath)) {
				continue;
			}
			try {
				QWidget* pageMainWidget = createNormalTab(dictPath);
				newDictNames.push_back(dictName.as_string());
				tabWidget->addTab(pageMainWidget, QString::fromStdString(dictName.as_string()));
			}
			catch (...) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("解析失败"), tr("默认译前字典 ") +
					QString::fromStdString(dictName.as_string()) + tr(" 不符合规范"), 3000);
				continue;
			}
		}
		insertToml(_globalConfig, _modeConfig + ".dictNames", newDictNames);
	}

	tabWidget->setCurrentIndex(0);
	int curIdx = tabWidget->currentIndex();
	if (curIdx >= 0) {
		removeTabButton->setEnabled(true);
	}
	else {
		removeTabButton->setEnabled(false);
	}

	connect(tabWidget, &ElaTabWidget::currentChanged, this, [=](int index)
		{
			if (index < 0) {
				removeTabButton->setEnabled(false);
			}
			else {
				removeTabButton->setEnabled(true);
			}
		});


	connect(importButton, &ElaPushButton::clicked, this, [=]()
		{
			QString dictPathStr = QFileDialog::getOpenFileName(this, tr("选择字典文件"), "./",
				"TOML files (*.toml);;JSON files (*.json)");
			if (dictPathStr.isEmpty()) {
				return;
			}
			fs::path dictPath = dictPathStr.toStdWString();
			fs::path newDictPath = L"BaseConfig/Dict/" + ascii2Wide(_modePath) + L"/" + dictPath.stem().wstring() + L".toml";
			if (fs::exists(newDictPath) && !fs::equivalent(dictPath, newDictPath)) {
				fs::remove(newDictPath);
			}
			bool hasSameNameTab = std::ranges::any_of(_normalTabEntries, [=](const NormalTabEntry& entry)
				{
					return entry.dictPath.stem().wstring() == dictPath.stem().wstring();
				});
			if (hasSameNameTab) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("导入失败"), tr("字典 ") +
					QString(dictPath.stem().wstring()) + tr(" 已存在"), 3000);
				return;
			}
			QWidget* pageMainWidget = createNormalTab(dictPath);
			tabWidget->addTab(pageMainWidget, QString(dictPath.stem().wstring()));
			tabWidget->setCurrentIndex(tabWidget->count() - 1);
			ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("创建成功"), tr("字典页 ") + QString(dictPath.stem().wstring()) + tr(" 已创建"), 3000);
		});

	connect(addNewTabButton, &ElaPushButton::clicked, this, [=]()
		{
			QString dictName;
			bool ok;
			ElaInputDialog inputDialog(_mainWindow, tr("请输入字典表名称"), tr("新建字典"), dictName, &ok);
			inputDialog.exec();

			if (!ok) {
				return;
			}
			if (dictName.isEmpty() || dictName.contains('/') || dictName.contains('\\') || dictName.contains('.')) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft,
					tr("新建失败"), tr("字典名称不能为空，且不能包含点号、斜杠或反斜杠！"), 3000);
				return;
			}

			fs::path newDictPath = L"BaseConfig/Dict/" + ascii2Wide(_modePath) + L"/" + dictName.toStdWString() + L".toml";
			bool hasSameNameTab = std::ranges::any_of(_normalTabEntries, [=](const NormalTabEntry& entry)
				{
					return entry.dictPath.stem().wstring() == dictName.toStdWString();
				});
			if (hasSameNameTab || dictName == "项目字典_译前" || dictName == "项目字典_译后") {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("新建失败"), tr("字典 ") +
					QString(newDictPath.stem().wstring()) + tr(" 已存在"), 3000);
				return;
			}

			std::ofstream ofs(newDictPath);
			if (!ofs.is_open()) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("新建失败"), tr("无法创建 ") +
					QString(newDictPath.wstring()) + tr(" 文件"), 3000);
				return;
			}
			ofs.close();

			QWidget* pageMainWidget = createNormalTab(newDictPath);
			tabWidget->addTab(pageMainWidget, dictName);
			tabWidget->setCurrentIndex(tabWidget->count() - 1);
			ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("新建成功"), tr("字典页 ") +
				QString(newDictPath.stem().wstring()) + tr(" 已创建"), 3000);
		});

	connect(removeTabButton, &ElaPushButton::clicked, this, [=]()
		{
			int index = tabWidget->currentIndex();
			if (index < 0) {
				ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("移除失败"), tr("请先选择一个字典页！"), 3000);
				return;
			}
			QWidget* pageMainWidget = tabWidget->currentWidget();
			auto it = std::ranges::find_if(_normalTabEntries, [=](const NormalTabEntry& entry)
				{
					return entry.pageMainWidget == pageMainWidget;
				});

			if (!pageMainWidget || it == _normalTabEntries.end()) {
				return;
			}

			std::string dictName = wide2Ascii(it->dictPath.stem().wstring());

			// 删除提示框
			ElaContentDialog helpDialog(_mainWindow);

			helpDialog.setRightButtonText(tr("是"));
			helpDialog.setMiddleButtonText(tr("思考人生"));
			helpDialog.setLeftButtonText(tr("否"));

			QWidget* widget = new QWidget(&helpDialog);
			QVBoxLayout* layout = new QVBoxLayout(widget);
			ElaText* confirmText = new ElaText(tr("你确定要删除 ") + QString::fromStdString(dictName) + " 吗？", 18, widget);
			confirmText->setWordWrap(false);
			layout->addWidget(confirmText);
			layout->addWidget(new ElaText(tr("将永久删除该字典文件，如有需要请先备份！"), 16, widget));
			helpDialog.setCentralWidget(widget);

			connect(&helpDialog, &ElaContentDialog::rightButtonClicked, this, [=]()
				{
					pageMainWidget->deleteLater();
					tabWidget->removeTab(index);
					fs::remove(it->dictPath);
					_normalTabEntries.erase(it);
					auto& dictNames = _globalConfig[_modeConfig]["dictNames"];
					if (dictNames.is_array()) {
						auto it = std::ranges::find_if(dictNames.as_array(), [=](const auto& elem)
							{
								return elem.is_string() && elem.as_string() == dictName;
							});
						if (it != dictNames.as_array().end()) {
							dictNames.as_array().erase(it);
						}
					}
					Q_EMIT commonDictsChanged();
					ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("删除成功"), tr("字典 ")
						+ QString::fromStdString(dictName) + tr(" 已从字典管理和磁盘中移除！"), 3000);
				});
			helpDialog.exec();
		});


	_applyFunc = [=]()
		{
			toml::array dictNamesArr;
			for (const NormalTabEntry& entry : _normalTabEntries) {
				
				std::ofstream ofs(entry.dictPath);
				if (!ofs.is_open()) {
					ElaMessageBar::error(ElaMessageBarType::TopLeft, tr("保存失败"), tr("无法打开字典: ") +
						QString(entry.dictPath.wstring()) + tr(" ，将跳过该字典的保存"), 3000);
					continue;
				}

				std::string dictName = wide2Ascii(entry.dictPath.stem().wstring());
				dictNamesArr.push_back(dictName);

				if (entry.stackedWidget->currentIndex() == 0) {
					ofs << entry.plainTextEdit->toPlainText().toStdString();
					ofs.close();
					QList<NormalDictEntry> newDictEntries = ReadDicts::readNormalDicts(entry.dictPath);
					entry.dictModel->loadData(newDictEntries);
				}
				else if (entry.stackedWidget->currentIndex() == 1) {
					QList<NormalDictEntry> dictEntries = entry.dictModel->getEntries();
					toml::value dictArr= toml::array{};
					for (const auto& entry : dictEntries) {
						toml::ordered_table dictTable;
						dictTable.insert({ "org", entry.original.toStdString() });
						dictTable.insert({ "rep", entry.translation.toStdString() });
						dictTable.insert({ "conditionTarget", entry.conditionTar.toStdString() });
						dictTable.insert({ "conditionReg", entry.conditionReg.toStdString() });
						dictTable.insert({ "isReg", entry.isReg });
						dictTable.insert({ "priority", entry.priority });
						dictArr.push_back(dictTable);
					}
					ofs << toml::format(toml::value{ toml::table{{"normalDict", dictArr}} });
					ofs.close();
					entry.plainTextEdit->setPlainText(ReadDicts::readDictsStr(entry.dictPath));
				}

				insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".openMode", entry.stackedWidget->currentIndex());
				insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".columnWidth.0", entry.tableView->columnWidth(0));
				insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".columnWidth.1", entry.tableView->columnWidth(1));
				insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".columnWidth.2", entry.tableView->columnWidth(2));
				insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".columnWidth.3", entry.tableView->columnWidth(3));
				insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".columnWidth.4", entry.tableView->columnWidth(4));
				insertToml(_globalConfig, _modeConfig + ".spec." + dictName + ".columnWidth.5", entry.tableView->columnWidth(5));
			}
			insertToml(_globalConfig, _modeConfig + ".dictNames", dictNamesArr);

			auto& spec = _globalConfig[_modeConfig]["spec"];
			if (spec.is_table()) {
				for (const auto& [key, value] : spec.as_table()) {
					if (
						!std::ranges::any_of(dictNamesArr, [&](const auto& elem)
							{
								return elem.is_string() && elem.as_string() == key;
							})
						)
					{
						spec.as_table().erase(key);
					}
				}
			}
			Q_EMIT commonDictsChanged();
		};


	mainLayout->addWidget(tabWidget, 1);
	addCentralWidget(mainWidget, true, true, 0);
}
