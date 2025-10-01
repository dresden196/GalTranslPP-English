// NJCfgPage.h

#ifndef NJCFGPAGE_H
#define NJCFGPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class NJCfgPage : public BasePage
{
    Q_OBJECT

public:
    explicit NJCfgPage(toml::value& projectConfig, QWidget* parent = nullptr);
    ~NJCfgPage();

private:
    toml::value& _projectConfig;
};

#endif // NJCFGPAGE_H
