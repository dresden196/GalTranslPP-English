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
        "1. 修复update点击卡片重启后会弹出remove update冲突",
        "2. DumpName改为仅更新模式(出现的原有人名只更新出现次数并保留翻译，未出现的原有人名会被删除)，不再覆盖原有文件。",
        "3. 修复通知栏有两个图标的bug",
        "4. 换行修复增加报错阈值功能",
        "5. 添加多语言选项(尚未完全完成)",
        "6. 添加自动检查更新选项",
        "7. 关于页面可跳转至发布页",
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
