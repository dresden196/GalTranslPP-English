// CommonGptDictPage.h

#ifndef COMMONGPTDICTPAGE_H
#define COMMONGPTDICTPAGE_H

#include <QList>
#include <QStackedWidget>
#include <QSharedPointer>
#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"
#include "DictionaryModel.h"

class ElaPlainTextEdit;
class ElaTableView;

namespace fs = std::filesystem;

struct GptTabEntry {
    QWidget* pageMainWidget;
    QStackedWidget* stackedWidget;
    ElaPlainTextEdit* plainTextEdit;
    ElaTableView* tableView;
    DictionaryModel* dictModel;
    fs::path dictPath;
    QSharedPointer<QList<DictionaryEntry>> withdrawList;
    GptTabEntry() : withdrawList(new QList<DictionaryEntry>){}
};

class CommonGptDictPage : public BasePage
{
    Q_OBJECT

public:
    explicit CommonGptDictPage(toml::ordered_value& globalConfig, QWidget* parent = nullptr);
    ~CommonGptDictPage() override;

Q_SIGNALS:
    void commonDictsChanged();

private:

    void _setupUI();

    toml::ordered_value& _globalConfig;

    QList<GptTabEntry> _gptTabEntries;
    QWidget* _mainWindow;
};

#endif // COMMONGPTDICTPAGE_H
