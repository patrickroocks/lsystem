#pragma once

#include <QQuickWidget>

class PlayerControl : public QQuickWidget
{
	Q_OBJECT
public:
	explicit PlayerControl(QWidget *parent);

	void setValue(int value);
	void setMaxValueAndValue(int maxValue, int value);
	void setPlaying(bool playing);

	void stashState();
	void unstashState();

signals:
	// when the user clicks in the timeline, first raise playPauseChanged(false), i.e., not playing,
	// then jump to the value, i.e.
	void playerValueChanged(int value);
	void playPauseChanged(bool playing);

private slots:
	// from QML
	void playingChanged();
	void valueChanged();

private:
	QQuickItem * control_ = nullptr;
	bool valueChanging = false;

	bool curPlaying = false;
	int curValue = 0;
	int curMaxValue = 0;

	struct StashedState
	{
		bool playing = false;
		int value = 0;
	};

	std::optional<StashedState> stashedState;
};
