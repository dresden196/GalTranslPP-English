#include "DictSettingsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QStackedWidget>
#include <QFileDialog>
#include <nlohmann/json.hpp>

#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaIconButton.h"
#include "ElaTabWidget.h"
#include "ElaScrollPageArea.h"
#include "ElaToolTip.h"
#include "ElaTableView.h"
#include "ElaPushButton.h"
#include "ElaMultiSelectComboBox.h"
#include "ElaMessageBar.h"
#include "ElaToggleSwitch.h"
#include "ElaPlainTextEdit.h"
#include "ReadDicts.h"

import Tool;
using json = nlohmann::json;

DictSettingsPage::DictSettingsPage(fs::path& projectDir, toml::value& globalConfig, toml::value& projectConfig, QWidget* parent) :
	BasePage(parent), _projectConfig(projectConfig), _globalConfig(globalConfig), _projectDir(projectDir)
{
	setWindowTitle(tr("项目字典设置"));
	setTitleVisible(false);
	setContentsMargins(0, 0, 0, 0);

	_setupUI();
}

DictSettingsPage::~DictSettingsPage()
{

}

void DictSettingsPage::refreshDicts()
{
	if (_refreshFunc) {
		_refreshFunc();
	}
}


void DictSettingsPage::_setupUI()
{
	QWidget* mainWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);

	ElaTabWidget* tabWidget = new ElaTabWidget(mainWidget);
	tabWidget->setTabsClosable(false);
	tabWidget->setIsTabTransparent(true);


	auto createDictTabFunc =
		[=]<typename ModelType, typename EntryType>(std::function<QString()> readPlainTextFunc, std::function<QList<EntryType>()> readEntriesFunc,
			QList<EntryType>& withdrawList, const std::string& configKey,
			const QString& tabName, const ModelType& dummy) -> std::pair<std::function<void()>, std::function<void(bool)>>
	{
		QWidget* gptDictWidget = new QWidget(mainWidget);
		QVBoxLayout* gptDictLayout = new QVBoxLayout(gptDictWidget);
		QWidget* gptButtonWidget = new QWidget(mainWidget);
		QHBoxLayout* gptButtonLayout = new QHBoxLayout(gptButtonWidget);
		ElaPushButton* gptPlainTextModeButtom = new ElaPushButton(gptButtonWidget);
		gptPlainTextModeButtom->setText(tr("纯文本模式"));
		ElaPushButton* gptTableModeButtom = new ElaPushButton(gptButtonWidget);
		gptTableModeButtom->setText(tr("表模式"));

		ElaIconButton* saveGptDictButton = new ElaIconButton(ElaIconType::Check, gptButtonWidget);
		saveGptDictButton->setFixedWidth(30);
		ElaToolTip* saveGptDictButtonToolTip = new ElaToolTip(saveGptDictButton);
		saveGptDictButtonToolTip->setToolTip(tr("保存当前页"));
		ElaIconButton* importGptDictButton = new ElaIconButton(ElaIconType::ArrowDownFromLine, gptButtonWidget);
		importGptDictButton->setFixedWidth(30);
		ElaToolTip* importGptDictButtonToolTip = new ElaToolTip(importGptDictButton);
		importGptDictButtonToolTip->setToolTip(tr("导入字典页"));
		ElaIconButton* withdrawGptDictButton = new ElaIconButton(ElaIconType::ArrowLeft, gptButtonWidget);
		withdrawGptDictButton->setFixedWidth(30);
		ElaToolTip* withdrawGptDictButtonToolTip = new ElaToolTip(withdrawGptDictButton);
		withdrawGptDictButtonToolTip->setToolTip(tr("撤回删除行"));
		withdrawGptDictButton->setEnabled(false);
		ElaIconButton* refreshGptDictButton = new ElaIconButton(ElaIconType::ArrowRotateRight, gptButtonWidget);
		refreshGptDictButton->setFixedWidth(30);
		ElaToolTip* refreshGptDictButtonToolTip = new ElaToolTip(refreshGptDictButton);
		refreshGptDictButtonToolTip->setToolTip(tr("刷新当前页"));
		ElaIconButton* addGptDictButton = new ElaIconButton(ElaIconType::Plus, gptButtonWidget);
		addGptDictButton->setFixedWidth(30);
		ElaToolTip* addGptDictButtonToolTip = new ElaToolTip(addGptDictButton);
		addGptDictButtonToolTip->setToolTip(tr("添加词条"));
		ElaIconButton* delGptDictButton = new ElaIconButton(ElaIconType::Minus, gptButtonWidget);
		delGptDictButton->setFixedWidth(30);
		ElaToolTip* delGptDictButtonToolTip = new ElaToolTip(delGptDictButton);
		delGptDictButtonToolTip->setToolTip(tr("删除词条"));
		gptButtonLayout->addWidget(gptPlainTextModeButtom);
		gptButtonLayout->addWidget(gptTableModeButtom);
		gptButtonLayout->addStretch();
		gptButtonLayout->addWidget(saveGptDictButton);
		gptButtonLayout->addWidget(importGptDictButton);
		gptButtonLayout->addWidget(withdrawGptDictButton);
		gptButtonLayout->addWidget(refreshGptDictButton);
		gptButtonLayout->addWidget(addGptDictButton);
		gptButtonLayout->addWidget(delGptDictButton);
		gptDictLayout->addWidget(gptButtonWidget, 0, Qt::AlignTop);


		// 每个字典里又有一个StackedWidget区分表和纯文本
		QStackedWidget* gptStackedWidget = new QStackedWidget(gptDictWidget);
		// 项目GPT字典的纯文本模式
		ElaPlainTextEdit* gptPlainTextEdit = new ElaPlainTextEdit(gptStackedWidget);
		QFont plainTextFont = gptPlainTextEdit->font();
		plainTextFont.setPixelSize(15);
		gptPlainTextEdit->setFont(plainTextFont);
		gptPlainTextEdit->setPlainText(readPlainTextFunc());
		gptStackedWidget->addWidget(gptPlainTextEdit);

		// 项目GPT字典的表格模式
		ElaTableView* gptDictTableView = new ElaTableView(gptStackedWidget);
		ModelType* gptDictModel = new ModelType(gptDictTableView);
		QList<EntryType> gptData = readEntriesFunc();
		gptDictModel->loadData(gptData);
		gptDictTableView->setModel(gptDictModel);
		QFont tableHeaderFont = gptDictTableView->horizontalHeader()->font();
		tableHeaderFont.setPixelSize(16);
		gptDictTableView->horizontalHeader()->setFont(tableHeaderFont);
		gptDictTableView->verticalHeader()->setHidden(true);
		gptDictTableView->setAlternatingRowColors(true);
		gptDictTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

		if constexpr (std::is_same_v<EntryType, DictionaryEntry>) {
			gptDictTableView->setColumnWidth(0, toml::get_or(_projectConfig["GUIConfig"]["gptDictTableColumnWidth"]["0"], 175));
			gptDictTableView->setColumnWidth(1, toml::get_or(_projectConfig["GUIConfig"]["gptDictTableColumnWidth"]["1"], 175));
			gptDictTableView->setColumnWidth(2, toml::get_or(_projectConfig["GUIConfig"]["gptDictTableColumnWidth"]["2"], 425));
		}
		else {
			gptDictTableView->setColumnWidth(0, toml::get_or(_projectConfig["GUIConfig"][configKey + "DictTableColumnWidth"]["0"], 200));
			gptDictTableView->setColumnWidth(1, toml::get_or(_projectConfig["GUIConfig"][configKey + "DictTableColumnWidth"]["1"], 150));
			gptDictTableView->setColumnWidth(2, toml::get_or(_projectConfig["GUIConfig"][configKey + "DictTableColumnWidth"]["2"], 100));
			gptDictTableView->setColumnWidth(3, toml::get_or(_projectConfig["GUIConfig"][configKey + "DictTableColumnWidth"]["3"], 172));
			gptDictTableView->setColumnWidth(4, toml::get_or(_projectConfig["GUIConfig"][configKey + "DictTableColumnWidth"]["4"], 75));
			gptDictTableView->setColumnWidth(5, toml::get_or(_projectConfig["GUIConfig"][configKey + "DictTableColumnWidth"]["5"], 60));
		}

		gptStackedWidget->addWidget(gptDictTableView);
		gptStackedWidget->setCurrentIndex(toml::get_or(_projectConfig["GUIConfig"][configKey + "DictTableOpenMode"], toml::get_or(_globalConfig["defaultDictOpenMode"], 0)));
		gptPlainTextModeButtom->setEnabled(gptStackedWidget->currentIndex() != 0);
		gptTableModeButtom->setEnabled(gptStackedWidget->currentIndex() != 1);
		addGptDictButton->setEnabled(gptStackedWidget->currentIndex() == 1);
		delGptDictButton->setEnabled(gptStackedWidget->currentIndex() == 1);

		gptDictLayout->addWidget(gptStackedWidget, 1);
		auto refreshGptDictFunc = [=]()
			{
				gptPlainTextEdit->setPlainText(readPlainTextFunc());
				gptDictModel->loadData(readEntriesFunc());
				ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("刷新成功"), tr("重新载入了 ") + tabName, 3000);
			};
		auto saveGptDictFunc = [=](bool forceSaveInTableModeToInit)
			{
				if constexpr (std::is_same_v<EntryType, DictionaryEntry>) {
					std::ofstream ofs(_projectDir / (tabName.toStdWString() + L".toml"));
					if (fs::exists(_projectDir / L"项目GPT字典-生成.toml")) {
						fs::remove(_projectDir / L"项目GPT字典-生成.toml");
					}
					if (gptStackedWidget->currentIndex() == 0 && !forceSaveInTableModeToInit) {
						ofs << gptPlainTextEdit->toPlainText().toStdString();
						ofs.close();
						gptDictModel->loadData(ReadDicts::readGptDicts(_projectDir / (tabName.toStdWString() + L".toml")));
					}
					else if (gptStackedWidget->currentIndex() == 1 || forceSaveInTableModeToInit) {
						toml::value gptDictArr = toml::array{};
						const QList<EntryType>& gptEntries = gptDictModel->getEntriesRef();
						for (const auto& entry : gptEntries) {
							toml::ordered_table gptDictTbl;
							gptDictTbl.insert({ "org", entry.original.toStdString() });
							gptDictTbl.insert({ "rep", entry.translation.toStdString() });
							gptDictTbl.insert({ "note", entry.description.toStdString() });
							gptDictArr.push_back(gptDictTbl);
						}
						gptDictArr.as_array_fmt().fmt = toml::array_format::multiline;
						ofs << toml::format(toml::value{ toml::table{{"gptDict", gptDictArr}} });
						ofs.close();
						gptPlainTextEdit->setPlainText(ReadDicts::readDictsStr(_projectDir / (tabName.toStdWString() + L".toml")));
					}
					insertToml(_projectConfig, "GUIConfig.gptDictTableColumnWidth.0", gptDictTableView->columnWidth(0));
					insertToml(_projectConfig, "GUIConfig.gptDictTableColumnWidth.1", gptDictTableView->columnWidth(1));
					insertToml(_projectConfig, "GUIConfig.gptDictTableColumnWidth.2", gptDictTableView->columnWidth(2));
					insertToml(_projectConfig, "GUIConfig.gptDictTableOpenMode", gptStackedWidget->currentIndex());
				}
				else {
					std::ofstream ofs(_projectDir / (tabName.toStdWString() + L".toml"));
					if (gptStackedWidget->currentIndex() == 0 && !forceSaveInTableModeToInit) {
						ofs << gptPlainTextEdit->toPlainText().toStdString();
						ofs.close();
						gptDictModel->loadData(readEntriesFunc());
					}
					else if (gptStackedWidget->currentIndex() == 1 || forceSaveInTableModeToInit) {
						toml::value postDictArr = toml::array{};
						const QList<EntryType>& postEntries = gptDictModel->getEntriesRef();
						for (const auto& entry : postEntries) {
							toml::ordered_table postDictTbl;
							postDictTbl.insert({ "org", entry.original.toStdString() });
							postDictTbl.insert({ "rep", entry.translation.toStdString() });
							postDictTbl.insert({ "conditionTarget", entry.conditionTar.toStdString() });
							postDictTbl.insert({ "conditionReg", entry.conditionReg.toStdString() });
							postDictTbl.insert({ "isReg", entry.isReg });
							postDictTbl.insert({ "priority", entry.priority });
							postDictArr.push_back(postDictTbl);
						}
						ofs << toml::format(toml::value{ toml::table{{"normalDict", postDictArr}} });
						ofs.close();
						gptPlainTextEdit->setPlainText(readPlainTextFunc());
					}
					insertToml(_projectConfig, "GUIConfig." + configKey + "DictTableColumnWidth.0", gptDictTableView->columnWidth(0));
					insertToml(_projectConfig, "GUIConfig." + configKey + "DictTableColumnWidth.1", gptDictTableView->columnWidth(1));
					insertToml(_projectConfig, "GUIConfig." + configKey + "DictTableColumnWidth.2", gptDictTableView->columnWidth(2));
					insertToml(_projectConfig, "GUIConfig." + configKey + "DictTableColumnWidth.3", gptDictTableView->columnWidth(3));
					insertToml(_projectConfig, "GUIConfig." + configKey + "DictTableColumnWidth.4", gptDictTableView->columnWidth(4));
					insertToml(_projectConfig, "GUIConfig." + configKey + "DictTableColumnWidth.5", gptDictTableView->columnWidth(5));
				}
			};
		connect(importGptDictButton, &ElaIconButton::clicked, this, [=]()
			{
				QString filter;
				if constexpr (std::is_same_v<EntryType, DictionaryEntry>) {
					filter = "TOML files (*.toml);;JSON files (*.json);;TSV files (*.tsv *.txt)";
				}
				else {
					filter = "TOML files (*.toml);;JSON files (*.json)";
				}
				QString dictPathStr = QFileDialog::getOpenFileName(this, tr("选择字典文件"), QString(_projectDir.wstring()), filter);
				if (dictPathStr.isEmpty()) {
					return;
				}
				fs::path dictPath = dictPathStr.toStdWString();
				QList<EntryType> entries = readEntriesFunc();
				if (entries.isEmpty()) {
					ElaMessageBar::warning(ElaMessageBarType::TopLeft, tr("导入失败"), tr("字典文件中没有词条"), 3000);
					return;
				}
				gptDictModel->loadData(entries);
				saveGptDictFunc(true);
				ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("导入成功"), tr("从文件 ") +
					QString(dictPath.filename().wstring()) + tr(" 中导入了 ") + QString::number(entries.size()) + tr(" 个词条"), 3000);
			});
		connect(gptPlainTextModeButtom, &ElaPushButton::clicked, this, [=]()
			{
				gptStackedWidget->setCurrentIndex(0);
				addGptDictButton->setEnabled(false);
				delGptDictButton->setEnabled(false);
				gptPlainTextModeButtom->setEnabled(false);
				gptTableModeButtom->setEnabled(true);
				withdrawGptDictButton->setEnabled(false);
			});
		connect(gptTableModeButtom, &ElaPushButton::clicked, this, [=, &withdrawList]()
			{
				gptStackedWidget->setCurrentIndex(1);
				addGptDictButton->setEnabled(true);
				delGptDictButton->setEnabled(true);
				gptPlainTextModeButtom->setEnabled(true);
				gptTableModeButtom->setEnabled(false);
				withdrawGptDictButton->setEnabled(!withdrawList.empty());
			});
		connect(refreshGptDictButton, &ElaPushButton::clicked, this, refreshGptDictFunc);
		connect(saveGptDictButton, &ElaPushButton::clicked, this, [=]()
			{
				saveGptDictFunc(false);
				ElaMessageBar::success(ElaMessageBarType::TopLeft, tr("保存成功"), tr("已保存 ") + tabName, 3000);
			});
		connect(addGptDictButton, &ElaPushButton::clicked, this, [=]()
			{
				QModelIndexList index = gptDictTableView->selectionModel()->selectedIndexes();
				if (index.isEmpty()) {
					gptDictModel->insertRow(gptDictModel->rowCount());
				}
				else {
					gptDictModel->insertRow(index.first().row());
				}
			});
		connect(delGptDictButton, &ElaPushButton::clicked, this, [=, &withdrawList]()
			{
				QModelIndexList selectedRows = gptDictTableView->selectionModel()->selectedRows();
				const QList<EntryType>& entries = gptDictModel->getEntriesRef();
				std::ranges::sort(selectedRows, [](const QModelIndex& a, const QModelIndex& b)
					{
						return a.row() > b.row();
					});
				for (const QModelIndex& index : selectedRows) {
					if (withdrawList.size() > 100) {
						withdrawList.pop_front();
					}
					withdrawList.push_back(entries[index.row()]);
					gptDictModel->removeRow(index.row());
				}
				if (!withdrawList.empty()) {
					withdrawGptDictButton->setEnabled(true);
				}
			});
		connect(withdrawGptDictButton, &ElaPushButton::clicked, this, [=, &withdrawList]()
			{
				if (withdrawList.empty()) {
					return;
				}
				EntryType entry = withdrawList.front();
				withdrawList.pop_front();
				gptDictModel->insertRow(0, entry);
				if (withdrawList.empty()) {
					withdrawGptDictButton->setEnabled(false);
				}
			});

		tabWidget->addTab(gptDictWidget, tabName);
		return { refreshGptDictFunc, saveGptDictFunc };
	};


	const std::vector<fs::path> gptDictPaths = { (_projectDir / tr("项目GPT字典.toml").toStdWString()), (_projectDir / L"项目GPT字典-生成.toml") };
	std::function<QString()> gptReadPlainTextFunc = [=]() -> QString
		{
			return ReadDicts::readGptDictsStr(gptDictPaths);
		};
	std::function<QList<DictionaryEntry>()> gptReadEntriesFunc = [=]() -> QList<DictionaryEntry>
		{
			return ReadDicts::readGptDicts(gptDictPaths);
		};
	auto refreshAndSaveGptDictFunc = createDictTabFunc(gptReadPlainTextFunc, gptReadEntriesFunc, _withdrawGptList, "gpt", QString(gptDictPaths.front().stem().wstring()), DictionaryModel{});


	fs::path preDictPath = _projectDir / tr("项目译前字典.toml").toStdWString();
	std::function<QString()> preReadPlainTextFunc = [=]() -> QString
		{
			return ReadDicts::readDictsStr(preDictPath);
		};
	std::function<QList<NormalDictEntry>()> preReadEntriesFunc = [=]() -> QList<NormalDictEntry>
		{
			return ReadDicts::readNormalDicts(preDictPath);
		};
	auto refreshAndSavePreDictFunc = createDictTabFunc(preReadPlainTextFunc, preReadEntriesFunc, _withdrawPreList, "pre", QString(preDictPath.stem().wstring()), NormalDictModel{});
	

	fs::path postDictPath = _projectDir / tr("项目译后字典.toml").toStdWString();
	std::function<QString()> postReadPlainTextFunc = [=]() -> QString
		{
			return ReadDicts::readDictsStr(postDictPath);
		};
	std::function<QList<NormalDictEntry>()> postReadEntriesFunc = [=]() -> QList<NormalDictEntry>
		{
			return ReadDicts::readNormalDicts(postDictPath);
		};
	auto refreshAndSavePostDictFunc = createDictTabFunc(postReadPlainTextFunc, postReadEntriesFunc, _withdrawPostList, "post", QString(postDictPath.stem().wstring()), NormalDictModel{});


	_refreshFunc = [=]()
		{
			refreshAndSaveGptDictFunc.first();
			//refreshAndSavePreDictFunc.first();
			//refreshAndSavePostDictFunc.first();
		};


	_applyFunc = [=]()
		{
			refreshAndSaveGptDictFunc.second(false);
			refreshAndSavePreDictFunc.second(false);
			refreshAndSavePostDictFunc.second(false);
		};

	mainLayout->addWidget(tabWidget, 1);
	addCentralWidget(mainWidget, true, true, 0);
}
