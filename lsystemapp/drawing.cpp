#include "drawing.h"

using namespace lsystem::common;

namespace lsystem::ui {

DrawingFrameSummary DrawingFrame::toDrawingFrameSummary()
{
	return DrawingFrameSummary{.topLeft = topLeft + offset, .botRight = botRight + offset, .offset = offset, .config = config};
}

void DrawingFrame::expandSizeToSegments(const common::LineSegs & segs, double thickness)
{
	const int off = qCeil(thickness / 2.);
	for (const LineSeg & seg : segs) {
		const QLine ln = seg.lineNegY();
		// clang-format off
		updateRect(qMin(ln.x1(), ln.x2()) - off, qMin(ln.y1(), ln.y2()) - off,
				   qMax(ln.x1(), ln.x2()) + off, qMax(ln.y1(), ln.y2()) + off);
		// clang-format on
	}
}

void DrawingFrame::updateRect(double minX, double minY, double maxX, double maxY)
{
	if (minX < topLeft.x()) topLeft.setX(minX);
	if (minY < topLeft.y()) topLeft.setY(minY);
	if (maxX > botRight.x()) botRight.setX(maxX);
	if (maxY > botRight.y()) botRight.setY(maxY);
}

// ----------------------------------------------------------

DrawingFrame::DrawingFrame(const ExecResult & execResult, const QSharedPointer<AllDrawData> & data)
	: offset(data->uiDrawData.offset)
	, config(data->config)
	, metaData(data->meta)
	, paintLastIter(!execResult.segmentsLastIter.isEmpty() && metaData.lastIterOpacy > 0)
{
	if (paintLastIter) expandSizeToSegments(execResult.segmentsLastIter, metaData.thickness);
	expandSizeToSegments(execResult.segments, metaData.thickness);
}

// ----------------------------------------------------------

Drawing::Drawing(const ExecResult & execResult, const QSharedPointer<AllDrawData> & data)
	: DrawingFrame(execResult, data)
	, num(data->uiDrawData.drawingNumToEdit.value_or(0))
	, segments(execResult.segments)
	, actionColors(execResult.actionColors)
{
	const QPoint pSize = botRight - topLeft + QPoint(1, 1);
	image = QImage(QSize(pSize.x(), pSize.y()), QImage::Format_ARGB32);
	image.fill(qRgba(0, 0, 0, 0)); // transparent

	InternalMeta meta;
	meta.antiAliasing = metaData.antiAliasing;
	meta.thickness = metaData.thickness;
	meta.colorGradient = metaData.colorGradient;

	if (paintLastIter) {
		lastIterMeta = meta;
		lastIterMeta->opacityFactor = metaData.lastIterOpacy;
		drawSegments(execResult.segmentsLastIter, *lastIterMeta);
		lastIterImage = image;
	}
	mainMeta = meta;
	mainMeta.opacityFactor = metaData.opacity;
	drawSegments(segments, mainMeta);
}

void Drawing::drawSegments(const LineSegs & segs, const InternalMeta & meta) { drawSegmentRange(segs, 0, segs.size() - 1, meta); }

void Drawing::drawToImage(QImage & dstImage, bool isMarked, bool isHighlighted)
{
	QPainter painter(&dstImage);
	painter.drawImage(offset + topLeft, image);
	if (isMarked) {
		QPen pen;
		pen.setColor(QColor(0, 0, 255, 50));
		pen.setDashPattern({3, 3});
		pen.setWidth(2);
		painter.setPen(pen);
		painter.drawRect(QRect(offset + topLeft, offset + botRight));
	}
	if (isHighlighted) {
		QPen pen;
		pen.setColor(QColor(0, 0, 0, 30));
		pen.setWidth(1);
		painter.setPen(pen);
		QPoint dist(2, 2);
		painter.drawRect(QRect(offset + topLeft - dist, offset + botRight + dist));
	}
}

QPoint Drawing::size() const { return botRight - topLeft; }

bool Drawing::withinArea(const QPoint & pos) { return pos >= topLeft + offset && pos <= botRight + offset; }

DrawingSummary Drawing::toDrawingSummary()
{
	DrawingSummary rv;
	static_cast<DrawingFrameSummary &>(rv) = toDrawingFrameSummary();
	rv.drawingNum = num;
	rv.listIndex = listIndex;
	rv.segmentsCount = static_cast<int>(segments.count());
	rv.animStep = animState.inProgress ? animState.curSeg + 1 : static_cast<int>(segments.count());
	return rv;
}

void Drawing::drawSegmentRange(const common::LineSegs & segs, int numStart, int numEnd, const InternalMeta & meta)
{
	QPainter painter(&image);

	if (meta.antiAliasing) painter.setRenderHint(QPainter::Antialiasing);
	QPen pen;
	pen.setWidthF(meta.thickness);
	pen.setCapStyle(Qt::RoundCap);

	QVector<QColor> drawColors;
	for (QColor actionColorCopy : actionColors) {
		actionColorCopy.setAlphaF(meta.opacityFactor);
		drawColors.push_back(actionColorCopy);
	}

	int lastColorNum = -1;

	const auto itStart = segs.cbegin() + numStart;
	const auto itEnd = segs.cbegin() + numEnd + 1;

	for (auto it = itStart; it != itEnd; ++it) {
		const auto & seg = *it;

		if (meta.colorGradient) {
			// color gradient mode
			const double normalizedSegNum = static_cast<double>(it - segs.cbegin()) / static_cast<double>(segs.size());
			auto color = meta.colorGradient->colorAt(normalizedSegNum);
			if (meta.opacityFactor < 1) color.setAlphaF(meta.opacityFactor);
			pen.setColor(color);
			painter.setPen(pen); // this is necessary after setColor!
		} else if (static_cast<int>(seg.colorNum) != lastColorNum) {
			// use colors from the config, which were written to the segments
			pen.setColor(drawColors.at(seg.colorNum));
			lastColorNum = seg.colorNum;
			painter.setPen(pen); // this is necessary after setColor!
		}

		if (seg.isPoint()) {
			painter.drawPoint(seg.pointNegY() - topLeft);
		} else {
			painter.drawLine(seg.lineNegY() - topLeft);
		}
	}

	animState.curSeg = numEnd;

	usesOpacity = (mainMeta.opacityFactor > 0 && mainMeta.opacityFactor < 1)
				  || (lastIterMeta.has_value() && lastIterMeta->opacityFactor > 0 && lastIterMeta->opacityFactor < 1);
}

void Drawing::drawBasicImage()
{
	image.fill(qRgba(0, 0, 0, 0));
	QPainter painter(&image);
	if (lastIterMeta.has_value()) painter.drawImage(QPoint(0, 0), lastIterImage);
}

AnimatorResult Drawing::newAnimationStep(int step, bool relativeStep)
{
	bool restarted = false;
	const int lastStep = animState.curSeg + 1;
	int newStep = 0;

	if (!animState.inProgress) {
		// Start from begin
		newStep = relativeStep ? 1 : step;
		restarted = true;
		animState.inProgress = true;

	} else {
		newStep = relativeStep ? lastStep + step : step;

		if (newStep == lastStep) {
			return AnimatorResult{.nextStepResult = AnimatorResult::NextStepResult::Unchanged, .step = newStep};
		}
	}

	if (newStep < lastStep) restarted = true;

	const int maxStep = segments.size();

	bool stopped = false;
	if (newStep >= maxStep) {
		stopped = true;
		newStep = maxStep;
	}

	if (restarted) drawBasicImage();

	if (stopped) animState.inProgress = false;

	// If not restarted, we only draw the additional segments.
	const int newSeg = newStep - 1;
	const int firstSegToDraw = restarted ? 0 : animState.curSeg;
	// This runs in the same thread as the main UI. It would be difficult to parallelize this,
	// as we would have to wait for the drawing to be completed anyway before we could refresh the DrawArea widget.
	// The widget itself has to run in the same thread as the main UI (widgets are not allowed to be in a own thread in Qt).
	drawSegmentRange(segments, firstSegToDraw, newSeg, mainMeta);

	animState.curSeg = newSeg;

	AnimatorResult rv;
	if (stopped) {
		rv.nextStepResult = AnimatorResult::NextStepResult::Stopped;
	} else if (restarted) {
		rv.nextStepResult = AnimatorResult::NextStepResult::Restart;
	} else {
		// The optimization that the contents are kept is only available if opacity is disabled.
		rv.nextStepResult = usesOpacity ? AnimatorResult::NextStepResult::Restart : AnimatorResult::NextStepResult::AddedOnly;
	}

	rv.step = newStep;

	return rv;
}

} // namespace lsystem::ui
