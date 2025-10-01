// TLFCfgPage.h

#ifndef TLFCFGPAGE_H
#define TLFCFGPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class TLFCfgPage : public BasePage
{
    Q_OBJECT

public:
    explicit TLFCfgPage(toml::value& projectConfig, QWidget* parent = nullptr);
    ~TLFCfgPage();

private:
    toml::value& _projectConfig;

};

#endif // TLFCFGPAGE_H
