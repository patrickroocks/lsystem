#include "playercontrol.h"

#include <QQuickItem>

namespace {

const char * qmlSource = "qrc:/util/playercontrol.qml";

}

PlayerControl::PlayerControl(QWidget * parent)
	: QQuickWidget(parent)
{
	// https://stackoverflow.com/questions/44975226/qquickwidget-with-transparent-background
	setAttribute(Qt::WA_AlwaysStackOnTop);
	setAttribute(Qt::WA_TranslucentBackground);
	setClearColor(Qt::transparent);

	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(QUrl(QString::fromUtf8(qmlSource)));
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
