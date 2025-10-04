#include "CodePageCheckerCfgPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ElaLineEdit.h"
#include "ElaText.h"
#include "ElaToolTip.h"
#include "ElaScrollPageArea.h"

import Tool;

CodePageCheckerCfgPage::CodePageCheckerCfgPage(toml::ordered_value& projectConfig, QWidget* parent)
    : BasePage(parent), _projectConfig(projectConfig)
{
    setWindowTitle(tr("字符集检查器设置"));
    setContentsMargins(10, 0, 10, 0);

    // 创建中心部件和布局
    QWidget* centerWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centerWidget);

    // 标点符号转换配置
    std::string codePage = toml::get_or(_projectConfig["plugins"]["CodePageChecker"]["codePage"], "gbk");
    ElaScrollPageArea* codePageArea = new ElaScrollPageArea(this);
    QHBoxLayout* codePageLayout = new QHBoxLayout(codePageArea);
    ElaText* codePageText = new ElaText(tr("字符集名称"), codePageArea);
    codePageText->setTextPixelSize(16);
    codePageLayout->addWidget(codePageText);
    codePageLayout->addStretch();
    ElaLineEdit* codePageEdit = new ElaLineEdit(codePageArea);
    codePageEdit->setMaximumWidth(180);
    codePageEdit->setText(QString::fromStdString(codePage));
    codePageLayout->addWidget(codePageEdit);
    mainLayout->addWidget(codePageArea);


    _applyFunc = [=]
        {
            insertToml(_projectConfig, "plugins.CodePageChecker.codePage", codePageEdit->text().toStdString());
        };

    mainLayout->addStretch();
    centerWidget->setWindowTitle(tr("字符集检查器设置"));
    addCentralWidget(centerWidget, true, true, 0);
}

CodePageCheckerCfgPage::~CodePageCheckerCfgPage()
{

}
