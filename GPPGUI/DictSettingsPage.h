// DictSettingsPage.h

#ifndef DICTSETTINGSPAGE_H
#define DICTSETTINGSPAGE_H

#include <QList>
#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"
#include "DictionaryModel.h"
#include "NormalDictModel.h"

namespace fs = std::filesystem;

class QStackedWidget;

class DictSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit DictSettingsPage(fs::path& projectDir, toml::value& globalConfig, toml::value& projectConfig, QWidget* parent = nullptr);
    ~DictSettingsPage() override;
    void refreshDicts();

private:

    void _setupUI();
    toml::value& _globalConfig;
    toml::value& _projectConfig;
    fs::path& _projectDir;

    std::function<void()> _refreshFunc;

    QList<DictionaryEntry> _withdrawGptList;
    QList<NormalDictEntry> _withdrawPreList;
    QList<NormalDictEntry> _withdrawPostList;
};

#endif // COMMONSETTINGSPAGE_H
