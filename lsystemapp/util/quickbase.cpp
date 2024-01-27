#include "quickbase.h"

#include <util/divutils.h>

#include <QFocusEvent>
#include <QQuickItem>
#include <QKeyEvent>

QuickBase::QuickBase(QWidget * parent, const char * qmlSource)
	: QQuickWidget(parent)
{
	// https://stackoverflow.com/questions/44975226/qquickwidget-with-transparent-background
	setAttribute(Qt::WA_AlwaysStackOnTop);
	setAttribute(Qt::WA_TranslucentBackground);
	setClearColor(Qt::transparent);

	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(QUrl(QString::fromUtf8(qmlSource)));
	selector_ = rootObject();

	QObject::connect(selector_, SIGNAL(valueChanged()), this, SLOT(valueChangedInSelector()));
}

void QuickBase::placeAt(int x, int y)
{
	setGeometry(QRect(x, y, sizeX, sizeY));
}

void QuickBase::setValue(double newValue)
{
	QMetaObject::invokeMethod(selector_, "setExtValue", Q_ARG(QVariant, newValue), Q_ARG(QVariant, true));
}

void QuickBase::setLineEdit(QLineEdit * newLineEdit)
{
	lineEdit = newLineEdit;
}

void QuickBase::setSmallBigStep(double newSmallStep, double newBigStep)
{
	smallStep_ = newSmallStep;
	bigStep_ = newBigStep;
	selector_->setProperty("coarseStepSize", smallStep_);
	selector_->setProperty("fineStepSize", smallStep_ / 10);
}

void QuickBase::setMinMaxValue(double newMinValue, double newMaxValue)
{
	minValue_ = newMinValue;
	maxValue_ = newMaxValue;
	selector_->setProperty("minValue", newMinValue);
	selector_->setProperty("maxValue", newMaxValue);
}

void QuickBase::focusOutEvent(QFocusEvent * event)
{
	QMetaObject::invokeMethod(selector_, "lostFocus");
	QQuickWidget::focusOutEvent(event);
	emit focusOut();
}

void QuickBase::keyPressEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Shift) {
		selector_->setProperty("rangeStepSize", bigStep_);
		QMetaObject::invokeMethod(selector_, "updateText");
	}

	const bool textHasFocus = selector_->property("textHasFocus").toBool();
	const bool pressedDown = event->key() == Qt::Key_Down || event->key() == Qt::Key_Minus || (!textHasFocus && event->key() == Qt::Key_Left);
	const bool pressedUp = event->key() == Qt::Key_Up || event->key() == Qt::Key_Plus || (!textHasFocus && event->key() == Qt::Key_Right);
	if (pressedDown || pressedUp) {
		const bool pressedShift = event->modifiers().testFlag(Qt::ShiftModifier);
		double newValue = currentValue + (pressedDown ? -1 : 1) * (pressedShift ? bigStep_ : smallStep_);
		if (pressedShift) {
			newValue = minValue_ + qRound((newValue - minValue_) / bigStep_) * bigStep_;
		}
		QMetaObject::invokeMethod(selector_, "setExtValue", Q_ARG(QVariant, newValue), Q_ARG(QVariant, false));
	} else {
		QQuickWidget::keyPressEvent(event);
	}
}

void QuickBase::keyReleaseEvent(QKeyEvent * event)
{
	if (event->key() == Qt::Key_Shift) {
		// freeze last value before setting the step back to small value
		QMetaObject::invokeMethod(selector_, "setExtValue", Q_ARG(QVariant, currentValue), Q_ARG(QVariant, false));
		selector_->setProperty("rangeStepSize", smallStep_);
		QMetaObject::invokeMethod(selector_, "updateText");
	}
	QQuickWidget::keyPressEvent(event);
}

void QuickBase::setSizeXY(int newSizeX, int newSizeY)
{
	sizeX = newSizeX;
	sizeY = newSizeY;
}

void QuickBase::valueChangedInSelector()
{
	const auto newValue = selector_->property("value").toDouble();
	if (newValue != currentValue) {
		currentValue = newValue;
		if (lineEdit) lineEdit->setText(util::formatFixed(currentValue, 2));
		emit valueChanged(currentValue);
	}
}
