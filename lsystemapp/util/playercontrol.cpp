#include "playercontrol.h"

#include <util/numericutils.h>

#include <QQuickItem>

namespace {

const char * QmlSource = "qrc:/util/playercontrol.qml";

const int BigStep = 5;
const int MinValue = 1;

} // namespace

PlayerControl::PlayerControl(QWidget * parent)
	: QQuickWidget(parent)
{
	// https://stackoverflow.com/questions/44975226/qquickwidget-with-transparent-background
	setAttribute(Qt::WA_AlwaysStackOnTop);
	setAttribute(Qt::WA_TranslucentBackground);
	setClearColor(Qt::transparent);

	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(QUrl(QString::fromUtf8(QmlSource)));
	control_ = rootObject();

	QObject::connect(control_, SIGNAL(playingChanged()), this, SLOT(playingChanged()));
	QObject::connect(control_, SIGNAL(valueChanged()), this, SLOT(valueChanged()));
}

void PlayerControl::setValue(int value)
{
	if (value == curValue) return;
	valueChanging = true;
	curValue = value;
	control_->setProperty("value", value);
	valueChanging = false;
}

void PlayerControl::keyPressEvent(QKeyEvent * event)
{
	const bool pressedDown = event->key() == Qt::Key_Down || event->key() == Qt::Key_Minus || event->key() == Qt::Key_Left;
	const bool pressedUp = event->key() == Qt::Key_Up || event->key() == Qt::Key_Plus || event->key() == Qt::Key_Right;
	if (pressedDown || pressedUp) {
		const bool pressedShift = event->modifiers().testFlag(Qt::ShiftModifier);
		int newValue = curValue + (pressedDown ? -1 : 1) * (pressedShift ? BigStep : 1);
		if (pressedShift) {
			newValue = static_cast<int>(qRound(1.0 * newValue / BigStep)) * BigStep;
		}
		newValue = util::ensureRange(newValue, MinValue, curMaxValue);
		control_->setProperty("value", newValue);
	} else {
		QQuickWidget::keyPressEvent(event);
	}
}

void PlayerControl::setMaxValueAndValue(int maxValue, int value)
{
	curMaxValue = maxValue;
	control_->setProperty("maxValue", maxValue);
	setValue(value);
}

void PlayerControl::setPlaying(bool playing)
{
	if (playing == curPlaying) return;
	valueChanging = true;
	curPlaying = playing;
	control_->setProperty("playing", playing);
	valueChanging = false;
}

void PlayerControl::playingChanged()
{
	if (valueChanging) return;
	curPlaying = control_->property("playing").toBool();
	emit playPauseChanged(curPlaying);
}

void PlayerControl::valueChanged()
{
	if (valueChanging) return;
	curValue = control_->property("value").toInt();
	emit playerValueChanged(curValue);
}

void PlayerControl::stashState()
{
	stashedState = StashedState{.playing = curPlaying, .value = curValue};
	setPlaying(false);

	emit playPauseChanged(curPlaying);
	emit playerValueChanged(curValue);
}

void PlayerControl::unstashState()
{
	if (!stashedState) return;

	if (stashedState->value <= curMaxValue) setValue(stashedState->value);
	setPlaying(stashedState->playing);

	emit playerValueChanged(curValue);
	emit playPauseChanged(curPlaying);

	stashedState = {};
}
