// CustomFilePluginCfgPage.h

#ifndef CUSTOMFILEPLUGINCFGPAGE_H
#define CUSTOMFILEPLUGINCFGPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class CustomFilePluginCfgPage : public BasePage
{
    Q_OBJECT

public:
    explicit CustomFilePluginCfgPage(toml::ordered_value& projectConfig, toml::ordered_value& globalConfig, QWidget* parent = nullptr);
    ~CustomFilePluginCfgPage();

private:
    toml::ordered_value& _projectConfig;
    toml::ordered_value& _globalConfig;
};

#endif // CUSTOMFILEPLUGINCFGPAGE_H
