// PromptSettingsPage.h

#ifndef PROMPTSSETTINGSPAGE_H
#define PROMPTSSETTINGSPAGE_H

#include <QList>
#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"
#include "GptDictModel.h"
#include "NormalDictModel.h"

namespace fs = std::filesystem;

class PromptSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit PromptSettingsPage(fs::path& projectDir, toml::ordered_value& projectConfig, QWidget* parent = nullptr);
    ~PromptSettingsPage() override;

private:

    void _setupUI();
    toml::ordered_value _promptConfig;
    toml::ordered_value& _projectConfig;
    fs::path& _projectDir;

};

#endif // PROMPTSSETTINGSPAGE_H
