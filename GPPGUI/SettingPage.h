#ifndef SETTINGPAGE_H
#define SETTINGPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class SettingPage : public BasePage
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit SettingPage(toml::value& globalConfig, QWidget* parent = nullptr);
    ~SettingPage() override;

private:

    toml::value& _globalConfig;
};

#endif // SETTINGPAGE_H
