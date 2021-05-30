#include "drawingcollection.h"
#include <util/qpointenhance.h>
#include <QPainter>

using namespace lsystem::common;

namespace lsystem::ui {

Drawing Drawing::fromSegments(const LineSegs & segs)
{
	Drawing rv;

	// determine size
	for (const LineSeg & seg : segs) {
		const QPoint start = seg.start.toPoint();
		const QPoint end   = seg.end.toPoint();
		rv.updateRect(qMin(start.x(), end.x()), qMin(start.y(), end.y()),
					  qMax(start.x(), end.x()), qMax(start.y(), end.y()));
	}

	// draw the segments
	const QPoint pSize = rv.botRight - rv.topLeft + QPoint(1, 1);
	rv.image = QImage(QSize(pSize.x(), pSize.y()), QImage::Format_ARGB32);
	rv.image.fill(qRgba(0, 0, 0, 0)); // transparent
	QPainter painter(&rv.image);
	QPen pen;

	for (const LineSeg & seg : segs) {
		pen.setColor(seg.color);
		painter.setPen(pen);
		const QPoint start = seg.start.toPoint() - rv.topLeft;
		const QPoint end   = seg.end  .toPoint() - rv.topLeft;
		painter.drawLine(start, end);
	}

	return rv;
}

void Drawing::drawToImage(QImage & dstImage, bool isMarked)
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
}

QPoint Drawing::size() const
{
	return botRight - topLeft;
}

bool Drawing::withinArea(const QPoint & pos)
{
	return pos >= topLeft + offset && pos <= botRight + offset;
}

void Drawing::move(const QPoint & newOffset)
{
	offset = newOffset;
}

void Drawing::updateRect(double minX, double minY, double maxX, double maxY)
{
	if (minX < topLeft .x()) topLeft .setX(minX);
	if (minY < topLeft .y()) topLeft .setY(minY);
	if (maxX > botRight.x()) botRight.setX(maxX);
	if (maxY > botRight.y()) botRight.setY(maxY);
}

// ----------------------------------------------------------------------------------------------------------------

void DrawingCollection::addDrawing(const LineSegs & segs, const QPoint & off)
{
	if (segs.isEmpty()) return;

	Drawing newDrawing = Drawing::fromSegments(segs);

	newDrawing.zIndex = zIndexToDrawing.isEmpty() ? 1 : (zIndexToDrawing.lastKey() + 1);
	newDrawing.num    = drawings       .isEmpty() ? 1 : (drawings       .lastKey() + 1);
	newDrawing.offset = off;

	zIndexToDrawing[newDrawing.zIndex] = newDrawing.num;
	drawings[newDrawing.num] = newDrawing;

	newDrawing.drawToImage(image, false);
}

void DrawingCollection::resize(const QSize & newSize)
{
	if (image.size() == newSize) return;

	QImage newImage(newSize, QImage::Format_RGB32);
	newImage.fill(backColor);
	QPainter painter(&newImage);
	painter.drawImage(QPoint(0, 0), image);
	image = newImage;
}

void DrawingCollection::clear()
{
	drawings.clear();
	zIndexToDrawing.clear();
	image.fill(backColor);
}

void DrawingCollection::redraw()
{
	dirty = false;
	image.fill(backColor);

	for (qint64 drawNum : zIndexToDrawing) {
		drawings[drawNum].drawToImage(image, drawNum == markedDrawing);
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

QImage & DrawingCollection::getImage(qint64 drawingNum)
{
	return drawings[drawingNum].image;
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
	drawings[drawingNum].move(newOffset);
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
