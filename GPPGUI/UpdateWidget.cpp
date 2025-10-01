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
        "0. 【重要！！！】大版本更新可能出现兼容性问题，建议备份当前尚未处理完毕的项目。",
        "0.5 【依然重要】由于项目译前字典和项目译后字典的默认文件名发送变化，你需要重命名或复制粘贴一下以重新导入字典。Epub正则表名同理",
        "1. 修复update点击卡片重启后会弹出remove update冲突",
        "2. DumpName改为仅更新模式(出现的原有人名只更新出现次数并保留翻译，未出现的原有人名会被删除)，不再覆盖原有文件。",
        "3. 修复通知栏有两个图标的bug",
        "4. 换行修复增加报错阈值功能",
        "5. 添加多语言选项",
        "6. 添加自动检查更新选项",
        "7. 关于页面可跳转至发布页",
        "8. toml++这个库真的很烂怂...",

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
