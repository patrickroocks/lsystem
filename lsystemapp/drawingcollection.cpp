#include "drawingcollection.h"

#include <util/qpointenhance.h>

using namespace lsystem::common;

namespace lsystem::ui {

Drawing::Drawing(const ExecResult & execResult, const QSharedPointer<ConfigAndMeta> & configAndMeta)
	: segments(execResult.segments)
	, actionColors(execResult.actionColors)
	, config(configAndMeta->config)
{
	const auto & metaData = configAndMeta->meta;

	const bool paintLastIter = !execResult.segmentsLastIter.isEmpty() && metaData.lastIterOpacy > 0;

	if (paintLastIter) expandSizeToSegments(execResult.segmentsLastIter, metaData.thickness);
	expandSizeToSegments(segments, metaData.thickness);

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

void Drawing::expandSizeToSegments(const common::LineSegs & segs, double thickness)
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

bool Drawing::move(const QPoint & newOffset)
{
	if (offset == newOffset) return false;
	offset = newOffset;
	return true;
}

DrawResult Drawing::toDrawResult()
{
	return DrawResult{.topLeft = topLeft + offset,
					  .botRight = botRight + offset,
					  .offset = offset,
					  .drawingNum = num,
					  .segmentsCount = static_cast<int>(segments.count()),
					  .animStep = animState.inProgress ? animState.curSeg + 1 : static_cast<int>(segments.count()),
					  .config = config};
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

void Drawing::updateRect(double minX, double minY, double maxX, double maxY)
{
	if (minX < topLeft.x()) topLeft.setX(minX);
	if (minY < topLeft.y()) topLeft.setY(minY);
	if (maxX > botRight.x()) botRight.setX(maxX);
	if (maxY > botRight.y()) botRight.setY(maxY);
}

// ----------------------------------------------------------------------------------------------------------------

void DrawingCollection::addDrawing(Drawing newDrawing, const QPoint & off)
{
	newDrawing.zIndex = zIndexToDrawing.isEmpty() ? 1 : (zIndexToDrawing.lastKey() + 1);
	newDrawing.num = drawings.isEmpty() ? 1 : (drawings.lastKey() + 1);
	newDrawing.offset = off;

	zIndexToDrawing[newDrawing.zIndex] = newDrawing.num;
	drawings[newDrawing.num] = newDrawing;

	newDrawing.drawToImage(image, false, false);
}

void DrawingCollection::resize(const QSize & newSize)
{
	QImage newImage(newSize, QImage::Format_RGB32);
	newImage.fill(backColor);
	QPainter painter(&newImage);
	painter.drawImage(QPoint(0, 0), image);
	image = newImage;
}

void DrawingCollection::clearAll()
{
	markedDrawing = 0;
	drawings.clear();
	zIndexToDrawing.clear();

	image.fill(backColor);
}

void DrawingCollection::deleteHighlightedOrLastDrawing()
{
	const auto numToDelete = getHighlightedDrawingNum() ? getHighlightedDrawingNum() : getLastDrawingNum();
	if (numToDelete == 0) return;

	drawings.remove(numToDelete);

	QMutableMapIterator it{zIndexToDrawing};
	while (it.hasNext()) {
		it.next();
		if (it.value() == numToDelete) it.remove();
	}
}

void DrawingCollection::redraw(bool keepContent)
{
	dirty = false;

	if (!keepContent) image.fill(backColor);

	for (qint64 drawNum : std::as_const(zIndexToDrawing)) {
		drawings[drawNum].drawToImage(image, !keepContent && drawNum == markedDrawing, !keepContent && drawNum == highlightedDrawing);
	}
}

QPoint DrawingCollection::getLastSize() const
{
	if (drawings.isEmpty()) return QPoint();
	return drawings.last().size();
}

qint64 DrawingCollection::getDrawingByPos(const QPoint & pos)
{
	if (zIndexToDrawing.isEmpty()) return 0;
	auto it = zIndexToDrawing.end();
	while (true) {
		it--;
		const qint64 drawNum = it.value();
		if (drawings[drawNum].withinArea(pos)) return drawNum;
		if (it == zIndexToDrawing.begin()) break;
	}

	return 0;
}

QPoint DrawingCollection::getDrawingOffset(qint64 drawingNum)
{
	if (!drawings.contains(drawingNum)) return QPoint();
	return drawings[drawingNum].offset;
}

QPoint DrawingCollection::getDrawingSize(qint64 drawingNum)
{
	if (!drawings.contains(drawingNum)) return QPoint();
	return drawings[drawingNum].size();
}

QImage & DrawingCollection::getDrawingImage(qint64 drawingNum) { return drawings[drawingNum].image; }

QImage DrawingCollection::getImage() { return image; }

Drawing * DrawingCollection::getCurrentDrawing()
{
	if (markedDrawing > 0) return &drawings[markedDrawing];
	if (drawings.isEmpty()) return nullptr;
	return &drawings.last();
}

bool DrawingCollection::setMarkedDrawing(qint64 newMarkedDrawing)
{
	if (markedDrawing == newMarkedDrawing) return false;

	markedDrawing = newMarkedDrawing;
	redraw();
	return true;
}

bool DrawingCollection::moveDrawing(qint64 drawingNum, const QPoint & newOffset)
{
	if (!drawings[drawingNum].move(newOffset)) return false;
	redraw();
	return true;
}

bool DrawingCollection::deleteImage(qint64 drawingNum)
{
	if (!drawings.contains(drawingNum)) return false;
	drawings.remove(drawingNum);
	redraw();
	return true;
}

bool DrawingCollection::sendToFront(qint64 drawingNum)
{
	if (zIndexToDrawing.isEmpty()) return false;
	return sendToZIndex(drawingNum, zIndexToDrawing.lastKey() + 1);
}

bool DrawingCollection::sendToBack(qint64 drawingNum)
{
	if (zIndexToDrawing.isEmpty()) return false;
	return sendToZIndex(drawingNum, zIndexToDrawing.firstKey() - 1);
}

bool DrawingCollection::highlightDrawing(qint64 newHighlightedDrawing)
{
	if (highlightedDrawing == newHighlightedDrawing) return false;
	highlightedDrawing = newHighlightedDrawing;
	redraw();
	return true;
}

std::optional<DrawResult> DrawingCollection::getHighlightedDrawResult()
{
	if (!highlightedDrawing) {
		return {};
	} else {
		return drawings[highlightedDrawing].toDrawResult();
	}
}

std::optional<DrawResult> DrawingCollection::getMarkedDrawResult()
{
	if (!markedDrawing) {
		return {};
	} else {
		return drawings[markedDrawing].toDrawResult();
	}
}

std::optional<QPoint> DrawingCollection::getLastOffset() const
{
	if (drawings.empty()) {
		return {};
	} else {
		return drawings.last().offset;
	}
}

bool DrawingCollection::sendToZIndex(qint64 drawingNum, qint64 newZIndex)
{
	if (!drawings.contains(drawingNum)) return false;
	Drawing & drawing = drawings[drawingNum];
	const qint64 oldZIndex = drawing.zIndex;
	drawing.zIndex = newZIndex;
	zIndexToDrawing.remove(oldZIndex);
	zIndexToDrawing[drawing.zIndex] = drawing.num;
	redraw();
	return true;
}

} // namespace lsystem::ui
