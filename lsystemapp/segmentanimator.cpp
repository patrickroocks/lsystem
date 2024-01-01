#include "segmentanimator.h"

#include <drawarea.h>

using namespace lsystem::common;

namespace lsystem {

SegmentAnimator::SegmentAnimator()
	: drawTimer(this)
{
	connect(&drawTimer, &QTimer::timeout, this, &SegmentAnimator::run);
	drawTimer.setSingleShot(true);
}

void SegmentAnimator::startAnimateCurrentDrawing()
{
	run();
}

void SegmentAnimator::setAnimateLatency(std::chrono::milliseconds latency)
{
	if (latency.count() > 0)
		latencyMs = latency.count();
}

void SegmentAnimator::stopAnimate()
{
	drawTimer.stop();
}

void SegmentAnimator::run()
{
	const auto startOfRun = QTime::currentTime();

	const auto res = emit newAnimationStep(1, true);

	const bool stopped = res.nextStepResult == AnimatorResult::NextStepResult::Stopped;
	if (!stopped) {
		const int reaminingLatencyMs = qMax(0, latencyMs - startOfRun.msecsTo(QTime::currentTime()));
		drawTimer.start(reaminingLatencyMs);
	}
}

void SegmentAnimator::goToAnimationStep(int step) { emit newAnimationStep(step, false); }

} // namespace lsystem
