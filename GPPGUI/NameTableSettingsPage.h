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
    explicit NameTableSettingsPage(fs::path& projectDir, toml::ordered_value& globalConfig, toml::ordered_value& projectConfig, QWidget* parent = nullptr);
    ~NameTableSettingsPage() override;
    void refreshTable();

private:

    void _setupUI();
    QList<NameTableEntry> readNameTable();
    QString readNameTableStr();
    toml::ordered_value& _projectConfig;
    toml::ordered_value& _globalConfig;
    fs::path& _projectDir;
    std::function<void()> _refreshFunc;

    QList<NameTableEntry> _withdrawList;
};

#endif // NAMETABLESETTINGSPAGE_H
