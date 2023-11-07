#include "segmentanimator.h"

#include <drawarea.h>

namespace lsystem {

SegmentAnimator::SegmentAnimator(ui::DrawArea * drawArea)
	: drawArea(drawArea),
	  drawTimer(this)
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
	using NextStepResult = ui::Drawing::NextStepResult;
	const NextStepResult result = drawArea->getCurrentDrawing()->nextAnimationStep();
	const bool stopped = result == NextStepResult::Stopped;
	const bool keepContent = result == NextStepResult::Continue;
	drawArea->redrawAndUpdate(keepContent);
	emit newAnimationStep(drawArea->getCurrentDrawing()->animState.curSeg + 1, stopped);
	if (!stopped)
		drawTimer.start(latencyMs);
}

void SegmentAnimator::goToAnimationStep(int step)
{
	if (drawArea->getCurrentDrawing()->goToAnimationStep(step))
		drawArea->redrawAndUpdate();
}

} // namespace lsystem
