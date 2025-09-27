#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <toml++/toml.hpp>
#include "BasePage.h"

class HomePage : public BasePage
{
    Q_OBJECT
public:

    Q_INVOKABLE explicit HomePage(toml::table& globalConfig, QWidget* parent = nullptr);
    ~HomePage();

private:

    toml::table& _globalConfig;
};

#endif // HOMEPAGE_H
