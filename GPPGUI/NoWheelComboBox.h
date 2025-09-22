#ifndef NOWHEELCOMBOBOX_H
#define NOWHEELCOMBOBOX_H

#include <QWheelEvent>
#include "ElaComboBox.h"

// 自定义ComboBox类
class NoWheelComboBox : public ElaComboBox
{
public:
	NoWheelComboBox(QWidget* parent = nullptr) : ElaComboBox(parent) {}

protected:
	// 重写滚轮事件
	void wheelEvent(QWheelEvent* event) override
	{
		// 忽略滚轮事件
		event->ignore();
	}
};

#endif // NOWHEELCOMBOBOX_H
