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
{}

void DrawArea::clear()
{
	clearAllDrawings();

	update();
	setNextUndoRedo(true);
}

void DrawArea::clearAllDrawings()
{
	if (drawings.highlightDrawing(0)) {
		emit highlightChanged({});
	}
	drawings.clearAll();
}

void DrawArea::draw(const ui::Drawing & drawing)
{
	drawings.addOrReplaceDrawing(drawing);

	update();
	setNextUndoRedo(true);
}

void DrawArea::restoreLastImage()
{
	drawings.restoreLast();
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
	drawings.deleteDrawing(drawings.getMarkedDrawingNum());
	drawings.setMarkedDrawing(0);
	update();
	setNextUndoRedo(true);
}

void DrawArea::deleteIndex(int index)
{
	auto drawingNum = drawings.getDrawingNumByListIndex(index);
	drawings.deleteDrawing(drawingNum);
	if (drawings.getMarkedDrawingNum() == drawingNum) drawings.setMarkedDrawing(0);
	update();
	setNextUndoRedo(true);
}

void DrawArea::sendToFrontMarked()
{
	drawings.sendToFront(drawings.getMarkedDrawingNum());
	update();
}

void DrawArea::sendToBackMarked()
{
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

void DrawArea::markHighlighted()
{
	if (drawings.getHighlightedDrawingNum() > 0) {
		markDrawing(drawings.getHighlightedDrawingNum());
	}
}

Drawing * DrawArea::getCurrentDrawing() { return drawings.getCurrentDrawing(); }

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
}

QColor DrawArea::getBgColor() const { return drawings.backColor; }

std::optional<DrawResult> DrawArea::getMarkedDrawingResult() { return drawings.getMarkedDrawResult(); }

AnimatorResult DrawArea::newAnimationStep(int step, bool relativeStep)
{
	auto * currentDrawing = getCurrentDrawing();
	if (!currentDrawing) return AnimatorResult{.nextStepResult = AnimatorResult::NextStepResult::Stopped};

	const auto res = currentDrawing->newAnimationStep(step, relativeStep);
	if (res.nextStepResult == AnimatorResult::NextStepResult::Unchanged) return res; // no painting
	redrawAndUpdate(res.nextStepResult == AnimatorResult::NextStepResult::AddedOnly);
	return res;
}

void DrawArea::layerSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	Q_UNUSED(deselected);

	if (ignoreSelectionChange) return;

	if (selected.indexes().empty()) {
		markDrawing(0);
	} else {
		markDrawing(drawings.getDrawingNumByListIndex(selected.indexes().first().row()));
	}
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
		update();
	}
	QWidget::resizeEvent(event);
}

void DrawArea::mousePressEvent(QMouseEvent * event)
{
	const qint64 clickedDrawing = drawings.getDrawingByPos(event->pos());

	bool cancelEvent = false;

	if (clickedDrawing > 0 && clickedDrawing == drawings.getMarkedDrawingNum() && event->button() == Qt::MouseButton::LeftButton) {
		move.mode = MoveState::ReadyForMove;
		move.start = event->position().toPoint();
		move.startOffset = drawings.getDrawingOffset(drawings.getMarkedDrawingNum()) - event->position().toPoint();
		setCursor(Qt::SizeAllCursor);
		cancelEvent = true;
	} else if (drawings.getMarkedDrawingNum() > 0 && clickedDrawing == 0) {
		cancelEvent = true;
	}

	markDrawing(clickedDrawing);

	if (!cancelEvent) {
		const auto pos = event->position().toPoint();
		emit mouseClick(pos.x(), pos.y(), event->button(), drawings.getMarkedDrawingNum() > 0);
	}
}

void DrawArea::markDrawing(int drawingNum)
{
	if (drawings.setMarkedDrawing(drawingNum)) {
		emit markingChanged();
		update();
	}
}

void DrawArea::mouseReleaseEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	if (move.mode != MoveState::NoMove) {
		setCursor(Qt::ArrowCursor);
		if (move.mode == MoveState::MoveStarted && drawings.getHighlightedDrawingNum()) {
			emit highlightChanged(drawings.getHighlightedDrawResult());
		}
		move.mode = MoveState::NoMove;
	}
}

void DrawArea::mouseMoveEvent(QMouseEvent * event)
{
	if (move.mode != MoveState::NoMove) {
		if (move.mode == MoveState::ReadyForMove) {
			drawings.storeUndoPoint();
			setNextUndoRedo(true);
			move.mode = MoveState::MoveStarted;
		}
		const QPoint newOffset = move.startOffset + event->position().toPoint();
		if (drawings.moveDrawing(drawings.getMarkedDrawingNum(), newOffset, false)) update();

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
