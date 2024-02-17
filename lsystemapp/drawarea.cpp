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

	drawings.clearAll();
	update();
	setNextUndoRedo(true);
}

void DrawArea::draw(const ui::Drawing & drawing, const QPoint & offset, bool clearAll, bool clearLast)
{
	lastDrawings = drawings;

	if (clearAll) {
		if (drawings.highlightDrawing(0)) {
			emit highlightChanged({});
		}
		drawings.clearAll();
	} else if (clearLast) {
		if (drawings.getHighlightedDrawingNum() == drawings.getLastDrawingNum() && drawings.highlightDrawing(0)) {
			emit highlightChanged({});
		}
		drawings.deleteHighlightedOrLastDrawing();
	}
	drawings.addDrawing(drawing, offset);

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
	const QPoint drawingSize = drawings.getDrawingSize(drawings.getMarkedDrawingNum());
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
	painter.drawImage(QPoint(0, 0), drawings.getDrawingImage(drawings.getMarkedDrawingNum()));
	clipboard->setImage(newImage);
}

void DrawArea::deleteMarked()
{
	lastDrawings = drawings;
	drawings.deleteImage(drawings.getMarkedDrawingNum());
	drawings.setMarkedDrawing(0);
	update();
	setNextUndoRedo(true);
}

void DrawArea::sendToFrontMarked()
{
	lastDrawings = drawings;
	drawings.sendToFront(drawings.getMarkedDrawingNum());
	update();
}

void DrawArea::sendToBackMarked()
{
	lastDrawings = drawings;
	drawings.sendToBack(drawings.getMarkedDrawingNum());
	update();
}

void DrawArea::translateHighlighted(const QPoint & newOffset)
{
	if (!drawings.getHighlightedDrawingNum()) return;
	if (drawings.moveDrawing(drawings.getHighlightedDrawingNum(), newOffset)) {
		update();
		emit highlightChanged(drawings.getHighlightedDrawResult());
	}
}

Drawing * DrawArea::getCurrentDrawing()
{
	return drawings.getCurrentDrawing();
}

void DrawArea::redrawAndUpdate(bool keepContent)
{
	drawings.redraw(keepContent);
	update();
	if (!keepContent && drawings.getHighlightedDrawingNum() > 0) {
		emit highlightChanged(drawings.getHighlightedDrawResult());
	}
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

std::optional<QPoint> DrawArea::getLastOffset() const
{
	return drawings.getLastOffset();
}

std::optional<DrawResult> DrawArea::getMarkedDrawingResult()
{
	return drawings.getMarkedDrawResult();
}

AnimatorResult DrawArea::newAnimationStep(int step, bool relativeStep)
{
	auto * currentDrawing = getCurrentDrawing();
	if (!currentDrawing) return AnimatorResult{.nextStepResult = AnimatorResult::NextStepResult::Stopped};

	const auto res = currentDrawing->newAnimationStep(step, relativeStep);
	redrawAndUpdate(res.nextStepResult != AnimatorResult::NextStepResult::Restart);
	return res;
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
		// avoid always resizing, when the window size is changed
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

	if (clickedDrawing > 0 && clickedDrawing == drawings.getMarkedDrawingNum() && event->button() == Qt::MouseButton::LeftButton) {
		moveMode = MoveState::ReadyForMove;
		moveStart = event->position().toPoint();
		moveStartOffset = drawings.getDrawingOffset(drawings.getMarkedDrawingNum()) - event->position().toPoint();
		setCursor(Qt::SizeAllCursor);
		cancelEvent = true;
	} else if (drawings.getMarkedDrawingNum() > 0 && clickedDrawing == 0) {
		cancelEvent = true;
	}

	if (drawings.setMarkedDrawing(clickedDrawing)) {
		emit markingChanged();
		update();
	}

	if (!cancelEvent) {
		const auto pos = event->position().toPoint();
		emit mouseClick(pos.x(), pos.y(), event->button(), drawings.getMarkedDrawingNum() > 0);
	}
}

void DrawArea::mouseReleaseEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	if (moveMode != MoveState::NoMove) {
		setCursor(Qt::ArrowCursor);
		if (moveMode == MoveState::MoveStarted && drawings.getHighlightedDrawingNum()) {
			emit highlightChanged(drawings.getHighlightedDrawResult());
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
		const QPoint newOffset = moveStartOffset + event->position().toPoint();
		if (drawings.moveDrawing(drawings.getMarkedDrawingNum(), newOffset)) update();

	} else {
		const qint64 mouseOverDrawing = drawings.getDrawingByPos(event->pos());
		if (drawings.getMarkedDrawingNum() == 0) {
			if (mouseOverDrawing > 0) {
				setCursor(Qt::ArrowCursor);
			} else {
				setCursor(Qt::CrossCursor);
			}
		}

		if (drawings.highlightDrawing(mouseOverDrawing)) {
			update();
			emit highlightChanged(drawings.getHighlightedDrawResult());
		}
	}
}

void DrawArea::setNextUndoRedo(bool undoOrRedo)
{
	nextUndoOrRedo = undoOrRedo;
	emit enableUndoRedo(nextUndoOrRedo);
}
} // namespace lsystem::ui
