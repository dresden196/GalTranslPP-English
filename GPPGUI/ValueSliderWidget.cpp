// ValueSliderWidget.cpp

#include "ValueSliderWidget.h"

#include <QHBoxLayout>
#include "ElaSlider.h"
#include "ElaDoubleSpinBox.h"

ValueSliderWidget::ValueSliderWidget(QWidget* parent, double minValue, double maxValue)
    : QWidget(parent), _minValue(minValue), _maxValue(maxValue)
{
    // 1. 创建子控件
    _slider = new ElaSlider(Qt::Horizontal, this);
    _spinBox = new ElaDoubleSpinBox(this);

    // 2. 设置子控件的属性
    _slider->setRange((int)(_minValue * 100), (int)(_maxValue * 100));

    _spinBox->setRange(_minValue, _maxValue);
    _spinBox->setDecimals(3); // 显示两位小数
    _spinBox->setSingleStep(0.001); // 每次点击上下箭头变化的步长
    _spinBox->setFixedWidth(140); // 给一个合适的固定宽度

    // 3. 创建布局并添加子控件
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(_slider);
    layout->addWidget(_spinBox);

    // 4. 建立内部信号槽连接
    connect(_slider, &ElaSlider::valueChanged, this, &ValueSliderWidget::_onSliderValueChanged);
    connect(_spinBox, QOverload<double>::of(&ElaDoubleSpinBox::valueChanged), this, &ValueSliderWidget::_onSpinBoxValueChanged);

    // 5. 设置初始值
    setValue((_minValue + _maxValue) / 2);
}

ValueSliderWidget::~ValueSliderWidget()
{
}

// 公共接口：设置值
void ValueSliderWidget::setValue(double value)
{
    value = qBound(_minValue, value, _maxValue);

    // 阻塞信号，防止在用代码设置值时触发无限循环
    _slider->blockSignals(true);
    _spinBox->blockSignals(true);

    _slider->setValue(static_cast<int>(value * 100));
    _spinBox->setValue(value);

    // 恢复信号
    _slider->blockSignals(false);
    _spinBox->blockSignals(false);

    // 发出一次信号，通知外部值已改变
    Q_EMIT valueChanged(value);
}

// 公共接口：获取值
double ValueSliderWidget::value() const
{
    return _spinBox->value();
}

void ValueSliderWidget::setDecimals(int decimals) {
    _spinBox->setDecimals(decimals);
    _spinBox->setSingleStep(pow(0.1, decimals));
}

// 私有槽：当滑块的值改变时
void ValueSliderWidget::_onSliderValueChanged(int intValue)
{
    // 将滑块的整数值 转换为浮点数值
    double doubleValue = intValue / 100.0;

    // 更新数字框的值，注意阻塞信号
    _spinBox->blockSignals(true);
    _spinBox->setValue(doubleValue);
    _spinBox->blockSignals(false);

    // 发出信号，通知外部
    Q_EMIT valueChanged(doubleValue);
}

// 私有槽：当数字框的值改变时
void ValueSliderWidget::_onSpinBoxValueChanged(double doubleValue)
{
    // 将数字框的浮点数值  转换为整数值 
    int intValue = static_cast<int>(doubleValue * 100);

    // 更新滑块的值，注意阻塞信号
    _slider->blockSignals(true);
    _slider->setValue(intValue);
    _slider->blockSignals(false);

    // 发出信号，通知外部
    Q_EMIT valueChanged(doubleValue);
}
