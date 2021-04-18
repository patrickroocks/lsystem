#include "clickablelabel.h"

ClickableLabel::ClickableLabel(QWidget * parent, Qt::WindowFlags f)
	: QLabel(parent)
{
	Q_UNUSED(f)
}

void ClickableLabel::mousePressEvent(QMouseEvent * event)
{
	emit mousePressed(event);
}
