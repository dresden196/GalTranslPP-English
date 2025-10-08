#include "PostFull2HalfCfgPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ElaToggleSwitch.h"
#include "ElaText.h"
#include "ElaToolTip.h"
#include "ElaScrollPageArea.h"

import Tool;

PostFull2HalfCfgPage::PostFull2HalfCfgPage(toml::ordered_value& projectConfig, QWidget* parent)
    : BasePage(parent), _projectConfig(projectConfig)
{
    setWindowTitle(tr("全角半角转换设置"));
    setContentsMargins(10, 0, 10, 0);

    // 创建中心部件和布局
    QWidget* centerWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centerWidget);

    // 标点符号转换配置
    bool convertPunctuation = toml::find_or(_projectConfig, "plugins", "TextPostFull2Half", "是否替换标点", true);
    ElaScrollPageArea* punctuationArea = new ElaScrollPageArea(centerWidget);
    QHBoxLayout* punctuationLayout = new QHBoxLayout(punctuationArea);
    ElaText* punctuationText = new ElaText(tr("转换标点符号"), punctuationArea);
    punctuationText->setWordWrap(false);
    punctuationText->setTextPixelSize(16);
    punctuationLayout->addWidget(punctuationText);
    punctuationLayout->addStretch();
    ElaToggleSwitch* punctuationSwitch = new ElaToggleSwitch(punctuationArea);
    punctuationSwitch->setIsToggled(convertPunctuation);
    punctuationLayout->addWidget(punctuationSwitch);
    mainLayout->addWidget(punctuationArea);

    // 反向替换配置
    bool reverseConvert = toml::find_or(_projectConfig, "plugins", "TextPostFull2Half", "是否反向替换", false);
    ElaScrollPageArea* reverseArea = new ElaScrollPageArea(centerWidget);
    QHBoxLayout* reverseLayout = new QHBoxLayout(reverseArea);
    ElaText* reverseText = new ElaText(tr("反向替换"), reverseArea);
    ElaToolTip* reverseTip = new ElaToolTip(reverseText);
    reverseTip->setToolTip(tr("关闭为全转半，开启为半转全"));
    reverseText->setTextPixelSize(16);
    reverseLayout->addWidget(reverseText);
    reverseLayout->addStretch();
    ElaToggleSwitch* reverseSwitch = new ElaToggleSwitch(reverseArea);
    reverseSwitch->setIsToggled(reverseConvert);
    reverseLayout->addWidget(reverseSwitch);

    _applyFunc = [=]
        {
            insertToml(_projectConfig, "plugins.TextPostFull2Half.是否替换标点", punctuationSwitch->getIsToggled());
            insertToml(_projectConfig, "plugins.TextPostFull2Half.是否反向替换", reverseSwitch->getIsToggled());
        };

    mainLayout->addWidget(reverseArea);
    mainLayout->addStretch();
    centerWidget->setWindowTitle(tr("全角半角转换设置"));
    addCentralWidget(centerWidget);
}

PostFull2HalfCfgPage::~PostFull2HalfCfgPage()
{

}
