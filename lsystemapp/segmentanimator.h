#pragma once

#include <QObject>
#include <QTimer>

namespace lsystem {

namespace ui {
class DrawArea;
class Drawing;
};

// Animator has to run in own thread
class SegmentAnimator : public QObject
{
	Q_OBJECT
public:
	SegmentAnimator(ui::DrawArea* drawArea);

public slots:
	void startAnimateCurrentDrawing();
	void setAnimateLatency(std::chrono::milliseconds latency);
	void stopAnimate();
	void goToAnimationStep(int step);

signals:
	void newAnimationStep(int step, bool animationDone);

private:
	void run();

private:
	ui::DrawArea* const drawArea;
	QTimer drawTimer;
	int latencyMs = 500;
};

} // namespace lsystem
