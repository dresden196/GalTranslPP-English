// CommonSettingsPage.h

#ifndef COMMONSETTINGSPAGE_H
#define COMMONSETTINGSPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class CommonSettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit CommonSettingsPage(toml::value& projectConfig, QWidget* parent = nullptr);
    ~CommonSettingsPage() override;

private:
    void _setupUI();
    toml::value& _projectConfig;
};

#endif // COMMONSETTINGSPAGE_H
