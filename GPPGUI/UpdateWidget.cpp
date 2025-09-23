#include "UpdateWidget.h"

#include <QVBoxLayout>
#include "ElaText.h"

import Tool;

UpdateWidget::UpdateWidget(QWidget* parent)
    : QWidget{parent}
{
    setMinimumSize(200, 260);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetMaximumSize);
    mainLayout->setContentsMargins(5, 10, 5, 5);
    mainLayout->setSpacing(4);

    ElaText* updateTitle = new ElaText("v" + QString::fromStdString(GPPVERSION) + " 更新", 15, this);
    QStringList updateList = {
        "1. 修复日志在达到滚动上限后无法保持原位的问题，增大GUI日志行容量至10000",
        "2. 修复竞态条件导致的重建显示失败(实际成功)的问题",
        "3. 将每个翻译模式的log分开，每个模式进行5个log的滚动",
        "4. 增加对names键的支持",
        "5. 将 retranslKeys 移至 problemAnalyze 表中",
        "6. 禁用了开始翻译页面的ComboBox的滚轮切换选项机能以防止误操作",
        "7. TextPostFull2Half 增加了对<br>和<tab>的忽略",
        "8. 支持自定义问题分析设置中的句子比较对象",
        "9. 将重翻关键字的编辑改为标准toml以支持特殊字符",

    };

    mainLayout->addWidget(updateTitle);
    for (const auto& str : updateList) {
        ElaText* updateItem = new ElaText(str, 13, this);
        updateItem->setIsWrapAnywhere(true);
        mainLayout->addWidget(updateItem);
    }
    
    mainLayout->addStretch();
}

UpdateWidget::~UpdateWidget()
{
}
