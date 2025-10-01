// PromptSettingsPage.h

#ifndef PROMPTSSETTINGSPAGE_H
#define PROMPTSSETTINGSPAGE_H

#include <QList>
#include <toml.hpp>
#include <filesystem>
#include "BasePage.h"
#include "DictionaryModel.h"
#include "NormalDictModel.h"

namespace fs = std::filesystem;

class PromptSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit PromptSettingsPage(fs::path& projectDir, toml::value& projectConfig, QWidget* parent = nullptr);
    ~PromptSettingsPage() override;

private:

    void _setupUI();
    toml::value _promptConfig;
    toml::value& _projectConfig;
    fs::path& _projectDir;

};

#endif // PROMPTSSETTINGSPAGE_H
