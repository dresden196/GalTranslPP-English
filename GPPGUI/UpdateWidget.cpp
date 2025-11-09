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
        "1. 向 Prompt 中实验性的添加了 动态背景 功能",
        "2. GenDict 的 Prompt 通用化",
        "3. 优化了一些界面细节",

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
