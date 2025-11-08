#include "ElaDoubleText.h"

#include <QVBoxLayout>
#include <QFormLayout>

ElaDoubleText::ElaDoubleText(QWidget* parent, const QString& firstLine, int firstLinePixelSize, const QString& secondLine, int secondLinePixelSize, const QString& toolTip)
	: QWidget(parent)
{
	QVBoxLayout* textLayout = new QVBoxLayout(this);
	ElaText* text = new ElaText(firstLine, firstLinePixelSize, this);
	text->setWordWrap(false);

	if (!toolTip.isEmpty()) {
		ElaToolTip* tip = new ElaToolTip(text);
		tip->setToolTip(toolTip);
	}
	textLayout->addWidget(text);

	if (!secondLine.isEmpty()) {
		ElaText* text2 = new ElaText(secondLine, secondLinePixelSize, this);
		text2->setWordWrap(false);
		textLayout->addWidget(text2);
	}
}

ElaDoubleText::~ElaDoubleText()
{

}
