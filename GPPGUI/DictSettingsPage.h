// DictSettingsPage.h

#ifndef DICTSETTINGSPAGE_H
#define DICTSETTINGSPAGE_H

#include <QList>
#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"
#include "GptDictModel.h"
#include "NormalDictModel.h"

namespace fs = std::filesystem;

class QStackedWidget;

class DictSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit DictSettingsPage(fs::path& projectDir, toml::ordered_value& globalConfig, toml::ordered_value& projectConfig, QWidget* parent = nullptr);
    ~DictSettingsPage() override;
    void refreshDicts();

private:

    void _setupUI();
    toml::ordered_value& _globalConfig;
    toml::ordered_value& _projectConfig;
    fs::path& _projectDir;

    std::function<void()> _refreshFunc;

    QList<GptDictEntry> _withdrawGptList;
    QList<NormalDictEntry> _withdrawPreList;
    QList<NormalDictEntry> _withdrawPostList;
};

#endif // COMMONSETTINGSPAGE_H
