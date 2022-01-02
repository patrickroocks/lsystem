#include "quicklinear.h"

#include <QFocusEvent>
#include <QQuickItem>
#include <QKeyEvent>

const char * QuickLinear::qmlSource = "qrc:/util/linearselector.qml";

QuickLinear::QuickLinear(QWidget * parent)
	: QQuickWidget(parent)
{
	// https://stackoverflow.com/questions/44975226/qquickwidget-with-transparent-background
	setAttribute(Qt::WA_AlwaysStackOnTop);
	setAttribute(Qt::WA_TranslucentBackground);
	setClearColor(Qt::transparent);

	setObjectName(QString::fromUtf8("quickLinearWidget"));
	setAutoFillBackground(true);
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(QUrl(QString::fromUtf8(qmlSource)));
	selector = rootObject();
	selector->setProperty("sizeWidth", sizeWidth);
	selector->setProperty("sizeHeight", sizeHeight);
	placeAt(0, 0);

	QObject::connect(selector, SIGNAL(valueChanged()), this, SLOT(valueChangedInSelector()));
}

void QuickLinear::placeAt(int x, int y)
{
	setGeometry(QRect(x, y, sizeWidth, sizeHeight));
}

void QuickLinear::setExtensionFactor(double extFactor)
{
	selector->setProperty("extensionFactor", extFactor);
}

void QuickLinear::setValue(double newValue)
{
	QMetaObject::invokeMethod(selector, "setExtValue", Q_ARG(QVariant, newValue), Q_ARG(QVariant, true));
}

void QuickLinear::setMinMaxValue(double newMinValue, double newMaxValue)
{
	minValue = newMinValue;
	maxValue = newMaxValue;
	selector->setProperty("minValue", newMinValue);
	selector->setProperty("maxValue", newMaxValue);
}

void QuickLinear::setSmallBigStep(double newSmallStep, double newBigStep)
{
	smallStep = newSmallStep;
	bigStep = newBigStep;
	selector->setProperty("rangeStepSize", smallStep);
}

void QuickLinear::setLineEdit(QLineEdit * newLineEdit)
{
	lineEdit = newLineEdit;
}

void QuickLinear::focusOutEvent(QFocusEvent * event)
{
	QMetaObject::invokeMethod(selector, "lostFocus");
	QQuickWidget::focusOutEvent(event);
	focusOut();
}

void QuickLinear::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Shift) {
		selector->setProperty("rangeStepSize", bigStep);
		QMetaObject::invokeMethod(selector, "updateText");
	}

	const bool textHasFocus = selector->property("textHasFocus").toBool();
	const bool pressedDown = event->key() == Qt::Key_Down || (!textHasFocus && event->key() == Qt::Key_Left);
	const bool pressedUp = event->key() == Qt::Key_Up || (!textHasFocus && event->key() == Qt::Key_Right);
	if (pressedDown || pressedUp) {
		const bool pressedShift = event->modifiers().testFlag(Qt::ShiftModifier);
		double newValue = currentValue;
		newValue += (pressedDown ? -1 : 1) * (pressedShift ? bigStep : smallStep);
		if (pressedShift) {
			newValue = minValue + qRound((newValue - minValue) / bigStep) * bigStep;
		}
		QMetaObject::invokeMethod(selector, "setExtValue", Q_ARG(QVariant, newValue), Q_ARG(QVariant, false));
	} else {
		QQuickWidget::keyPressEvent(event);
	}
}

void QuickLinear::keyReleaseEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Shift) {
		selector->setProperty("rangeStepSize", smallStep);
		QMetaObject::invokeMethod(selector, "updateText");
	}
	QQuickWidget::keyPressEvent(event);
}


void QuickLinear::valueChangedInSelector()
{
	const double newValue = selector->property("value").toDouble();
	if (newValue != currentValue) {
		currentValue = newValue;
		if (lineEdit) lineEdit->setText(QString::number(currentValue, 'g', 2));
		emit valueChanged(currentValue);
	}
}
