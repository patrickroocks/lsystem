#pragma once

#include <QQuickWidget>

class PlayerControl : public QQuickWidget
{
	Q_OBJECT
public:
	explicit PlayerControl(QWidget * parent);

signals:
	// when the user clicks in the timeline, first raise playPauseChanged(false), i.e., not playing,
	// then jump to the value, i.e.
	void valueChanged(int value);
	void playPauseChanged(bool playing);

private slots:
	// from QML
	void playingChanged();

private:
	QQuickItem * control_ = nullptr;
};
