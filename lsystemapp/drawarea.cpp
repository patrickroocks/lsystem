#include "drawarea.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>

using namespace lsystem::common;

namespace lsystem::ui {

DrawArea::DrawArea(QWidget * parent) : QWidget(parent)
{
}

void DrawArea::clear()
{
	lastDrawings = drawings;

	drawings.clear();
	update();
	setNextUndoRedo(true);
}

void DrawArea::draw(const ui::Drawing & drawing, int offX, int offY, bool clearBefore)
{
	lastDrawings = drawings;

	if (clearBefore) drawings.clear();
	drawings.addDrawing(drawing, QPoint(offX, offY));

	update();
	setNextUndoRedo(true);
}

void DrawArea::restoreLastImage()
{
	qSwap(drawings, lastDrawings);
	drawings.setMarkedDrawing(0);
	markedDrawing = 0;
	update();
	setNextUndoRedo(!nextUndoOrRedo);
}

void DrawArea::copyToClipboardFull()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setImage(drawings.image);
}

void DrawArea::copyToClipboardMarked(bool transparent)
{
	const QPoint drawingSize = drawings.getDrawingSize(markedDrawing);
	if (drawingSize.isNull()) return;

	QClipboard * clipboard = QGuiApplication::clipboard();
	const QPoint size = drawingSize + QPoint(1, 1);
	QImage newImage(QSize(size.x(), size.y()), QImage::Format_ARGB32);
	QPainter painter(&newImage);

	if (transparent) {
		newImage.fill(qRgba(0, 0, 0, 0)); // transparent
	} else {
		newImage.fill(drawings.backColor);
	}
	painter.drawImage(QPoint(0, 0), drawings.getImage(markedDrawing));
	clipboard->setImage(newImage);
}

void DrawArea::deleteMarked()
{
	lastDrawings = drawings;
	drawings.deleteImage(markedDrawing);
	update();
	setNextUndoRedo(true);
}

void DrawArea::sendToFrontMarked()
{
	lastDrawings = drawings;
	drawings.sendToFront(markedDrawing);
	update();
}

void DrawArea::sendToBackMarked()
{
	lastDrawings = drawings;
	drawings.sendToBack(markedDrawing);
	update();
}

QPoint DrawArea::getLastSize() const
{
	return drawings.getLastSize();
}

void DrawArea::setBgColor(const QColor & col)
{
	drawings.backColor = col;
	drawings.redraw();
	update();

	lastDrawings.backColor = col;
	lastDrawings.dirty = true;
}

QColor DrawArea::getBgColor() const
{
	return drawings.backColor;
}

void DrawArea::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	QRect dirtyRect = event->rect();
	painter.drawImage(dirtyRect, drawings.image, dirtyRect);
}

void DrawArea::resizeEvent(QResizeEvent * event)
{
	QImage & image = drawings.image;
	if (width() > image.width() || height() > image.height()) {
		// avoid always resizing, wehen the window size is changed
		int newWidth = qMax(width() + 128, image.width());
		int newHeight = qMax(height() + 128, image.height());
		const QSize newSize(newWidth, newHeight);
		drawings.resize(newSize);
		lastDrawings.resize(newSize);
		update();
	}
	QWidget::resizeEvent(event);
}

void DrawArea::mousePressEvent(QMouseEvent * event)
{
	const qint64 clickedDrawing = drawings.getDrawingByPos(event->pos());

	bool cancelEvent = false;

	if (clickedDrawing > 0 && clickedDrawing == markedDrawing && event->button() == Qt::MouseButton::LeftButton) {
		moveMode = MoveState::ReadyForMove;
		moveStartOffset = drawings.getDrawingOffset(markedDrawing) - QPoint(event->x(), event->y());
		setCursor(Qt::SizeAllCursor);
		cancelEvent = true;
	} else if (markedDrawing > 0 && clickedDrawing == 0) {
		cancelEvent = true;
	}

	markedDrawing = clickedDrawing;
	if (drawings.setMarkedDrawing(markedDrawing)) {
		emit markingChanged(markedDrawing > 0);
		update();
	}

	if (!cancelEvent) {
		emit mouseClick(event->x(), event->y(), event->button(), markedDrawing > 0);
	}
}

void DrawArea::mouseReleaseEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	if (moveMode != MoveState::NoMove) {
		setCursor(Qt::ArrowCursor);
		moveMode = MoveState::NoMove;
	}
}

void DrawArea::mouseMoveEvent(QMouseEvent * event)
{
	if (moveMode != MoveState::NoMove) {

		if (moveMode == MoveState::ReadyForMove) {
			lastDrawings = drawings;
			setNextUndoRedo(true);
			moveMode = MoveState::MoveStarted;
		}
		QPoint newOffset = moveStartOffset + QPoint(event->x(), event->y());
		if (drawings.moveDrawing(markedDrawing, newOffset)) update();

	} else {

		const qint64 mouseOverDrawing = drawings.getDrawingByPos(event->pos());
		if (markedDrawing == 0) {
			if (mouseOverDrawing > 0) {
				setCursor(Qt::ArrowCursor);
			} else {
				setCursor(Qt::CrossCursor);
			}
		}
	}
}

void DrawArea::setNextUndoRedo(bool undoOrRedo)
{
	nextUndoOrRedo = undoOrRedo;
	emit enableUndoRedo(nextUndoOrRedo);
}

}
