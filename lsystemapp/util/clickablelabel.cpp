#include "clickablelabel.h"

#include <QMouseEvent>

ClickableLabel::ClickableLabel(QWidget * parent, Qt::WindowFlags f)
	: QLabel(parent, f)
{
}

void ClickableLabel::mousePressEvent(QMouseEvent * event)
{
	emit mousePressed(event);
	QLabel::mousePressEvent(event);
	event->accept(); // prevents event in parent widget
}

void ClickableLabel::mouseMoveEvent(QMouseEvent * event)
{
	emit mouseMoved(event);
	QLabel::mouseMoveEvent(event);
	event->accept();
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent * event)
{
	emit mouseReleased(event);
	QLabel::mouseReleaseEvent(event);
	event->accept();
}
