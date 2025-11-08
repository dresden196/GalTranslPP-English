// ElaDoubleText.h

#ifndef ELADOUBLETEXT_H
#define ELADOUBLETEXT_H

#include "ElaText.h"
#include "ElaToolTip.h"

class ElaLineEdit;

class ElaDoubleText : public QWidget
{
    Q_OBJECT

public:
    explicit ElaDoubleText(QWidget* parent, const QString& firstLine, int firstLinePixelSize, const QString& secondLine, int secondLinePixelSize, const QString& toolTip);
    ~ElaDoubleText();

private:
    ElaText* _firstLine;
    ElaText* _secondLine;
    ElaToolTip* _toolTip;
};

#endif // ELADOUBLETEXT_H