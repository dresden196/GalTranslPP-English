#ifndef CODEPAGECHECKERCFGPAGE_H
#define CODEPAGECHECKERCFGPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class CodePageCheckerCfgPage : public BasePage
{
    Q_OBJECT

public:
    explicit CodePageCheckerCfgPage(toml::ordered_value& projectConfig, QWidget* parent = nullptr);
    ~CodePageCheckerCfgPage();

private:
    toml::ordered_value& _projectConfig;
};

#endif // ! CODEPAGECHECKERCFGPAGE_H
