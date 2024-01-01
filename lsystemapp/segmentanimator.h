#pragma once

#include <common.h>

#include <QObject>
#include <QTimer>

namespace lsystem {

namespace ui {
class DrawArea;
class Drawing;
};

class SegmentAnimator : public QObject
{
	Q_OBJECT
public:
	SegmentAnimator();

public slots:
	void startAnimateCurrentDrawing();
	void setAnimateLatency(std::chrono::milliseconds latency);
	void stopAnimate();
	void goToAnimationStep(int step);

signals:
	// We need the full namespace such that the Qt type system finds the type.
	lsystem::common::AnimatorResult newAnimationStep(int step, bool relativeStep);

private:
	void run();

private:
	QTimer drawTimer;
	int latencyMs = 500;
};

} // namespace lsystem
