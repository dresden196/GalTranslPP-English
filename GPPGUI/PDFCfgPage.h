// PDFCfgPage.h

#ifndef PDFCFGPAGE_H
#define PDFCFGPAGE_H

#include <toml.hpp>
#include "BasePage.h"

class PDFCfgPage : public BasePage
{
    Q_OBJECT

public:
    explicit PDFCfgPage(toml::value& projectConfig, QWidget* parent = nullptr);
    ~PDFCfgPage();

private:
    toml::value& _projectConfig;
};

#endif // PDFCFGPAGE_H
