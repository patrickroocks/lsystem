#include "drawingcollection.h"
#include <util/qpointenhance.h>
#include <QPainter>

using namespace lsystem::common;

namespace lsystem::ui {

Drawing::Drawing(const ExecResult & execResult, const QSharedPointer<MetaData> & metaData)
	: numSegments(execResult.segments.size())
	, config(metaData->config)
{
	const bool paintLastIter = !execResult.segmentsLastIter.isEmpty() && metaData->lastIterOpacy > 0;

	if (paintLastIter) expandSizeToSegments(execResult.segmentsLastIter, metaData->thickness);
	expandSizeToSegments(execResult.segments, metaData->thickness);

	const QPoint pSize = botRight - topLeft + QPoint(1, 1);
	image = QImage(QSize(pSize.x(), pSize.y()), QImage::Format_ARGB32);
	image.fill(qRgba(0, 0, 0, 0)); // transparent

	if (paintLastIter) drawSegments(execResult.segmentsLastIter, metaData->lastIterOpacy, metaData->thickness, metaData->antiAliasing);
	drawSegments(execResult.segments, metaData->opacity, metaData->thickness, metaData->antiAliasing);
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
	QPainter painter(&image);
	if (antiAliasing) painter.setRenderHint(QPainter::Antialiasing);
	QPen pen;
	pen.setWidthF(thickness);
	pen.setCapStyle(Qt::RoundCap);

	// todo order segments by color
	for (const LineSeg & seg : segs) {
		QColor colorCopy(seg.color);
		colorCopy.setAlphaF(opacyFactor);
		pen.setColor(colorCopy);

		painter.setPen(pen);
		painter.drawLine(seg.lineNegY() - topLeft);
	}
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

void DrawingCollection::redraw()
{
	dirty = false;
	image.fill(backColor);

	for (qint64 drawNum : qAsConst(zIndexToDrawing)) {
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
