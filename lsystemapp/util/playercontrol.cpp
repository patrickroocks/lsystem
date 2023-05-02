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

	setAutoFillBackground(true);
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(QUrl(QString::fromUtf8(qmlSource)));
	control_ = rootObject();

	QObject::connect(control_, SIGNAL(playingChanged()), this, SLOT(playingChanged()));
}

void PlayerControl::playingChanged()
{
	const auto playing = control_->property("playing").toBool();
	emit playPauseChanged(playing);
}