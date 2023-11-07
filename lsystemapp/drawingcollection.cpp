#include "drawingcollection.h"

#include <util/qpointenhance.h>

using namespace lsystem::common;

namespace lsystem::ui {

Drawing::Drawing(const ExecResult & execResult, const QSharedPointer<MetaData> & metaData)
	: segments(execResult.segments)
	, actionColors(execResult.actionColors)
	, config(metaData->config)
	, metaData(*metaData)
{
	const bool paintLastIter = !execResult.segmentsLastIter.isEmpty() && metaData->lastIterOpacy > 0;

	if (paintLastIter) expandSizeToSegments(execResult.segmentsLastIter, metaData->thickness);
	expandSizeToSegments(segments, metaData->thickness);

	const QPoint pSize = botRight - topLeft + QPoint(1, 1);
	image = QImage(QSize(pSize.x(), pSize.y()), QImage::Format_ARGB32);
	image.fill(qRgba(0, 0, 0, 0)); // transparent

	if (paintLastIter) drawSegments(execResult.segmentsLastIter, metaData->lastIterOpacy, metaData->thickness, metaData->antiAliasing);
	drawSegments(segments, metaData->opacity, metaData->thickness, metaData->antiAliasing);
}

void Drawing::expandSizeToSegments(const common::LineSegs & segs, double thickness)
{
	const int off = qCeil(thickness / 2.);
	for (const LineSeg & seg : segs) {
		const QLine ln = seg.lineNegY();
		updateRect(qMin(ln.x1(), ln.x2()) - off, qMin(ln.y1(), ln.y2()) - off,
				   qMax(ln.x1(), ln.x2()) + off, qMax(ln.y1(), ln.y2()) + off);
	}
}

void Drawing::drawSegments(const LineSegs & segs, double opacyFactor, double thickness, bool antiAliasing)
{
	drawSegmentRange(0, segs.size() - 1, opacyFactor, thickness, antiAliasing);
}

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

QPoint Drawing::size() const
{
	return botRight - topLeft;
}

bool Drawing::withinArea(const QPoint & pos)
{
	return pos >= topLeft + offset && pos <= botRight + offset;
}

bool Drawing::move(const QPoint & newOffset)
{
	if (offset == newOffset) return false;
	offset = newOffset;
	return true;
}

DrawResult Drawing::toDrawResult()
{
	return DrawResult{.topLeft = topLeft + offset, .botRight = botRight + offset, .offset = offset, .drawingNum = num, .config = config};
}

void Drawing::drawSegmentRange(int numStart, int numEnd, double opacyFactor, double thickness, bool antiAliasing)
{
	QPainter painter(&image);

	if (antiAliasing) painter.setRenderHint(QPainter::Antialiasing);
	QPen pen;
	pen.setWidthF(thickness);
	pen.setCapStyle(Qt::RoundCap);

	QVector<QColor> drawColors;
	for (QColor actionColorCopy : actionColors) {
		actionColorCopy.setAlphaF(opacyFactor);
		drawColors.push_back(actionColorCopy);
	}

	quint8 lastColorNum = 0;

	const auto itStart = segments.cbegin() + numStart;
	const auto itEnd = segments.cbegin() + numEnd + 1;

	for (auto it = itStart ; it != itEnd ; ++it) {
		const auto& seg = *it;

		if (seg.colorNum != lastColorNum) {
			pen.setColor(drawColors.at(seg.colorNum));
			lastColorNum = seg.colorNum;
			painter.setPen(pen);   // this is necessary after setColor!
		}

		if (seg.isPoint()) {
			painter.drawPoint(seg.pointNegY() - topLeft);
		} else {
			painter.drawLine(seg.lineNegY() - topLeft);
		}
	}

	animState.curSeg = numEnd;
}

Drawing::NextStepResult Drawing::nextAnimationStep()
{
	bool restarted = false;

	if (!animState.inProgress) {
		image.fill(qRgba(0, 0, 0, 0)); // transparent
		animState.curSeg = 0;
		animState.inProgress = true;
		restarted = true;
	} else if (animState.curSeg == segments.size() - 1) {
		animState.inProgress = false;
		return NextStepResult::Stopped;
	} else {
		animState.curSeg++;
	}

	drawSegmentRange(animState.curSeg, animState.curSeg, metaData.opacity, metaData.thickness, metaData.antiAliasing);

	return restarted ? NextStepResult::Restart : NextStepResult::Continue;
}

bool Drawing::goToAnimationStep(int newStep)
{
	const int newSegNum = newStep - 1;

	if (newSegNum < 0 || newSegNum >= segments.size())
		return false;

	if (animState.inProgress) {
		if (newSegNum > animState.curSeg) {
			// only draw additional segments
			drawSegmentRange(animState.curSeg, newSegNum, metaData.opacity, metaData.thickness, metaData.antiAliasing);
			return true;
		} else if (newSegNum == animState.curSeg) {
			return false;
		}
	}

	// start from the begin
	image.fill(qRgba(0, 0, 0, 0)); // transparent
	drawSegmentRange(0, newSegNum, metaData.opacity, metaData.thickness, metaData.antiAliasing);
	animState.inProgress = true;

	return true;
}

void Drawing::updateRect(double minX, double minY, double maxX, double maxY)
{
	if (minX < topLeft .x()) topLeft .setX(minX);
	if (minY < topLeft .y()) topLeft .setY(minY);
	if (maxX > botRight.x()) botRight.setX(maxX);
	if (maxY > botRight.y()) botRight.setY(maxY);
}

// ----------------------------------------------------------------------------------------------------------------

void DrawingCollection::addDrawing(Drawing newDrawing, const QPoint & off)
{
	newDrawing.zIndex = zIndexToDrawing.isEmpty() ? 1 : (zIndexToDrawing.lastKey() + 1);
	newDrawing.num    = drawings       .isEmpty() ? 1 : (drawings       .lastKey() + 1);
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

	if (!keepContent)
		image.fill(backColor);

	for (qint64 drawNum : std::as_const(zIndexToDrawing)) {
		drawings[drawNum].drawToImage(image, drawNum == markedDrawing, drawNum == highlightedDrawing);
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

QImage & DrawingCollection::getDrawingImage(qint64 drawingNum)
{
	return drawings[drawingNum].image;
}

QImage DrawingCollection::getImage()
{
	return image;
}

Drawing* DrawingCollection::getCurrentDrawing()
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

}
