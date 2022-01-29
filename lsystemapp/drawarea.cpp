#include "drawarea.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>

using namespace lsystem::common;

namespace lsystem::ui {

DrawArea::DrawArea(QWidget * parent)
	: QWidget(parent)
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

	if (clearBefore) {
		drawings.clear();
	}
	drawings.addDrawing(drawing, QPoint(offX, offY));

	// hack to ensure that the image is really painted
	// todo: find out what happens here, it's all in the same thread
	QThread::msleep(20);

	update();

	setNextUndoRedo(true);
}

void DrawArea::restoreLastImage()
{
	qSwap(drawings, lastDrawings);
	drawings.setMarkedDrawing(0);
	update();
	setNextUndoRedo(!nextUndoOrRedo);
}

void DrawArea::copyToClipboardFull()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setImage(drawings.getImage());
}

void DrawArea::copyToClipboardMarked(bool transparent)
{
	const QPoint drawingSize = drawings.getDrawingSize(drawings.getMarkedDrawing());
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
	painter.drawImage(QPoint(0, 0), drawings.getDrawingImage(drawings.getMarkedDrawing()));
	clipboard->setImage(newImage);
}

void DrawArea::deleteMarked()
{
	lastDrawings = drawings;
	drawings.deleteImage(drawings.getMarkedDrawing());
	drawings.setMarkedDrawing(0);
	update();
	setNextUndoRedo(true);
}

void DrawArea::sendToFrontMarked()
{
	lastDrawings = drawings;
	drawings.sendToFront(drawings.getMarkedDrawing());
	update();
}

void DrawArea::sendToBackMarked()
{
	lastDrawings = drawings;
	drawings.sendToBack(drawings.getMarkedDrawing());
	update();
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
	painter.drawImage(dirtyRect, drawings.getImage(), dirtyRect);
}

void DrawArea::resizeEvent(QResizeEvent * event)
{
	QImage image = drawings.getImage();
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

	if (clickedDrawing > 0 && clickedDrawing == drawings.getMarkedDrawing() && event->button() == Qt::MouseButton::LeftButton) {
		moveMode = MoveState::ReadyForMove;
		moveStart = QPoint(event->x(), event->y());
		moveStartOffset = drawings.getDrawingOffset(drawings.getMarkedDrawing()) - QPoint(event->x(), event->y());
		setCursor(Qt::SizeAllCursor);
		cancelEvent = true;
	} else if (drawings.getMarkedDrawing() > 0 && clickedDrawing == 0) {
		cancelEvent = true;
	}

	if (drawings.setMarkedDrawing(clickedDrawing)) {
		emit markingChanged(drawings.getMarkedDrawing() > 0);
		update();
	}

	if (!cancelEvent) {
		emit mouseClick(event->x(), event->y(), event->button(), drawings.getMarkedDrawing() > 0);
	}
}

void DrawArea::mouseReleaseEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	if (moveMode != MoveState::NoMove) {
		setCursor(Qt::ArrowCursor);
		if (moveMode == MoveState::MoveStarted) {
			const QPoint transPt = QPoint(event->x(), event->y()) - moveStart;
			emit translation(transPt.x(), transPt.y());
		}
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
		if (drawings.moveDrawing(drawings.getMarkedDrawing(), newOffset)) update();

	} else {

		const qint64 mouseOverDrawing = drawings.getDrawingByPos(event->pos());
		if (drawings.getMarkedDrawing() == 0) {
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
