// NameTableSettingsPage.h

#ifndef NAMETABLESETTINGSPAGE_H
#define NAMETABLESETTINGSPAGE_H

#include <QList>
#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"
#include "NameTableModel.h"

namespace fs = std::filesystem;

class NameTableSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit NameTableSettingsPage(fs::path& projectDir, toml::value& globalConfig, toml::value& projectConfig, QWidget* parent = nullptr);
    ~NameTableSettingsPage() override;
    void refreshTable();

private:

    void _setupUI();
    QList<NameTableEntry> readNameTable();
    QString readNameTableStr();
    toml::value& _projectConfig;
    toml::value& _globalConfig;
    fs::path& _projectDir;
    std::function<void()> _refreshFunc;

    QList<NameTableEntry> _withdrawList;
};

#endif // NAMETABLESETTINGSPAGE_H
