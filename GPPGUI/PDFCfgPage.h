// PDFCfgPage.h

#ifndef PDFCFGPAGE_H
#define PDFCFGPAGE_H

#include <toml++/toml.hpp>
#include "BasePage.h"

class PDFCfgPage : public BasePage
{
    Q_OBJECT

public:
    explicit PDFCfgPage(toml::table& projectConfig, QWidget* parent = nullptr);
    ~PDFCfgPage();

private:
    toml::table& _projectConfig;
};

#endif // PDFCFGPAGE_H
