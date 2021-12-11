#include "quickangle.h"

#include <QFocusEvent>
#include <QQuickItem>
#include <QKeyEvent>

const char * QuickAngle::qmlSource = "qrc:/util/angleselector.qml";

QuickAngle::QuickAngle(QWidget * parent)
	: QQuickWidget(parent)
{
	// https://stackoverflow.com/questions/44975226/qquickwidget-with-transparent-background
	setAttribute(Qt::WA_AlwaysStackOnTop);
	setAttribute(Qt::WA_TranslucentBackground);
	setClearColor(Qt::transparent);

	setObjectName(QString::fromUtf8("quickAngleWidget"));
	setAutoFillBackground(true);
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(QUrl(QString::fromUtf8(qmlSource)));
	selector = rootObject();
	selector->setProperty("size", size);
	placeAt(0, 0);

	QObject::connect(selector, SIGNAL(valueChanged()), this, SLOT(valueChangedInSelector()));
}

void QuickAngle::placeAt(int x, int y)
{
	setGeometry(QRect(x, y, size, size));
}

void QuickAngle::setValue(int newValue)
{
	QMetaObject::invokeMethod(selector, "setExtValue", Q_ARG(QVariant, newValue), Q_ARG(QVariant, true));
}

void QuickAngle::setLineEdit(QLineEdit * newLineEdit)
{
	lineEdit = newLineEdit;
}

void QuickAngle::setValueRestriction(ValueRestriction valueRestriction)
{
	const int valueFilter = [&valueRestriction]() {
			switch (valueRestriction) {
			case ValueRestriction::None:
			case ValueRestriction::Numbers:
				return 0;
			case ValueRestriction::NegativeNumbers:
				return -1;
			case ValueRestriction::PositiveNumbers:
				return 1;
			default:
				return 0;
			}
		}();

	selector->setProperty("valueFilter", valueFilter);
}

void QuickAngle::focusOutEvent(QFocusEvent * event)
{
	QMetaObject::invokeMethod(selector, "lostFocus");
	QQuickWidget::focusOutEvent(event);
	focusOut();
}

void QuickAngle::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Shift) {
		selector->setProperty("rangeStepSize", 5);
		QMetaObject::invokeMethod(selector, "updateText");
	}

	const bool textHasFocus = selector->property("textHasFocus").toBool();
	const bool pressedDown = event->key() == Qt::Key_Down || (!textHasFocus && event->key() == Qt::Key_Left);
	const bool pressedUp = event->key() == Qt::Key_Up || (!textHasFocus && event->key() == Qt::Key_Right);
	if (pressedDown || pressedUp) {
		const bool pressedShift = event->modifiers().testFlag(Qt::ShiftModifier);
		int newValue = currentValue;
		if (pressedDown) {
			newValue -= pressedShift ? 5 : 1;
		} else if (pressedUp) {
			newValue += pressedShift ? 5 : 1;
		}
		if (pressedShift) newValue = qRound(newValue / 5.0) * 5;
		QMetaObject::invokeMethod(selector, "setExtValue", Q_ARG(QVariant, newValue), Q_ARG(QVariant, false));
	} else {
		QQuickWidget::keyPressEvent(event);
	}
}

void QuickAngle::keyReleaseEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Shift) {
		selector->setProperty("rangeStepSize", 1);
		QMetaObject::invokeMethod(selector, "updateText");
	}
	QQuickWidget::keyPressEvent(event);
}

void QuickAngle::valueChangedInSelector()
{
	const int newValue = selector->property("value").toInt();
	if (newValue != currentValue) {
		currentValue = newValue;
		if (lineEdit) lineEdit->setText(QString::number(currentValue));
		emit valueChanged(currentValue);
	}
}
