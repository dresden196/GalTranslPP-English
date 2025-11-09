#include "ElaDoubleText.h"

#include <QVBoxLayout>
#include <QFormLayout>

ElaDoubleText::ElaDoubleText(QWidget* parent, const QString& firstLine, int firstLinePixelSize, const QString& secondLine, int secondLinePixelSize, const QString& toolTip)
	: QWidget(parent)
{
	QVBoxLayout* textLayout = new QVBoxLayout(this);
	_firstLine = new ElaText(firstLine, firstLinePixelSize, this);
	_firstLine->setWordWrap(false);

	if (!toolTip.isEmpty()) {
		_toolTip = new ElaToolTip(_firstLine);
		_toolTip->setToolTip(toolTip);
	}
	textLayout->addWidget(_firstLine);

	if (!secondLine.isEmpty()) {
		_secondLine = new ElaText(secondLine, secondLinePixelSize, this);
		_secondLine->setWordWrap(false);
		textLayout->addWidget(_secondLine);
	}
}

QString ElaDoubleText::getFirstLineText() const
{
	return _firstLine->text();
}

ElaDoubleText::~ElaDoubleText()
{

}
