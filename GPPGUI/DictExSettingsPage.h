#ifndef DICTEXSETTINGSPAGE_H
#define DICTEXSETTINGSPAGE_H

#include <QList>
#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"

namespace fs = std::filesystem;

class DictExSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit DictExSettingsPage(toml::value& globalConfig, toml::value& projectConfig, QWidget* parent = nullptr);
    ~DictExSettingsPage() override;
    void refreshCommonDictsList();

private:

    void _setupUI();
    toml::value& _globalConfig;
    toml::value& _projectConfig;

    std::function<void()> _refreshCommonDictsListFunc;
};

#endif // DICTEXSETTINGSPAGE_H