// OtherSettingsPage.h

#ifndef OTHERSETTINGSPAGE_H
#define OTHERSETTINGSPAGE_H

#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"
namespace fs = std::filesystem;

class OtherSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit OtherSettingsPage(QWidget* mainWindow, fs::path& projectDir, toml::ordered_value& projectConfig, QWidget* parent = nullptr);
    ~OtherSettingsPage() override;

Q_SIGNALS:
    void saveConfigSignal();
    void refreshProjectConfigSignal();

private:

    void _setupUI();
    toml::ordered_value& _projectConfig;

    fs::path& _projectDir;
    QWidget* _mainWindow;
};

#endif // OTHERSETTINGSPAGE_H
