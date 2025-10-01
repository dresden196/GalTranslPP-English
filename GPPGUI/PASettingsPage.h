// PASettingsPage.h

#ifndef PASETTINGSPAGE_H
#define PASETTINGSPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class PASettingsPage : public BasePage
{
    Q_OBJECT

public:
    explicit PASettingsPage(toml::value& projectConfig, QWidget* parent = nullptr);
    ~PASettingsPage() override;

private:

    void _setupUI();
    toml::value& _projectConfig;
};

#endif // COMMONSETTINGSPAGE_H
