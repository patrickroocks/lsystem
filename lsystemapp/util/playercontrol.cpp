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
	valueChanging = true;
	control_->setProperty("value", value);
	valueChanging = false;
}

void PlayerControl::setMaxValueAndValue(int maxValue)
{
	control_->setProperty("maxValue", maxValue);
	setValue(maxValue);
}

void PlayerControl::setPlaying(bool playing)
{
	control_->setProperty("playing", playing);
}

void PlayerControl::playingChanged()
{
	const auto playing = control_->property("playing").toBool();
	emit playPauseChanged(playing);
}

void PlayerControl::valueChanged()
{
	if (valueChanging) return;
	const auto value = control_->property("value").toInt();
	emit playerValueChanged(value);
}
