#include "focusablelineedit.h"

#include <QFocusEvent>

FocusableLineEdit::FocusableLineEdit(QWidget * parent)
	: QLineEdit(parent)
{
}

void FocusableLineEdit::focusInEvent(QFocusEvent * event)
{
	QLineEdit::focusInEvent(event);
	emit gotFocus(this);
}

void FocusableLineEdit::focusOutEvent(QFocusEvent * event)
{
	QLineEdit::focusOutEvent(event);
	emit lostFocus(this);
}
