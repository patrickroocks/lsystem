#include "drawarea.h"

#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>

using namespace lsystem::common;

namespace lsystem::ui {

DrawArea::DrawArea(QWidget * parent)
	: QWidget(parent)
	, ctxMenu(this)
{
	move.menu.addAction("move to here", this, &DrawArea::moveDrawingHere);
}

void DrawArea::clear()
{
	drawings.clearAll();
	emit highlightChanged({});
	update();
	setNextUndoRedo(true);
}

void DrawArea::moveDrawingHere()
{
	if (move.mode != MoveState::MoveByMenu) return;

	if (drawings.moveDrawing(drawings.getMarkedDrawingNum(), move.moveToPos, false)) update();
	move.mode = MoveState::NoMove;
}

void DrawArea::draw(const QSharedPointer<ui::Drawing> & drawing)
{
	drawings.addOrReplaceDrawing(drawing);

	// In general, the drawing dimensions changed, so the icons for the highlighted drawing have to be updated.
	emit highlightChanged(drawings.getHighlightedDrawResult());

	update();
	setNextUndoRedo(true);
}

void DrawArea::copyToClipboardFull()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setImage(drawings.getImage());
}

void DrawArea::deleteMarked()
{
	if (drawings.getMarkedDrawingNum() > 0) {
		deleteDrawing(drawings.getMarkedDrawingNum());
	}
}

void DrawArea::deleteIndex(int index) { deleteDrawing(drawings.getDrawingNumByListIndex(index)); }

void DrawArea::deleteDrawing(int drawingNum)
{
	const bool wasHighlighted = drawingNum == drawings.getHighlightedDrawingNum();
	drawings.deleteDrawing(drawingNum);
	if (wasHighlighted) emit highlightChanged({});
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

std::optional<DrawingSummary> DrawArea::getMarkedDrawingResult() { return drawings.getMarkedDrawResult(); }

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
		move.startOffset = drawings.getDrawingOffset(drawings.getMarkedDrawingNum()) - event->position().toPoint();
		move.mode = MoveState::ReadyForMove;
		setCursor(Qt::ClosedHandCursor);
		cancelEvent = true;
	} else if (drawings.getMarkedDrawingNum() > 0 && clickedDrawing == 0) {
		if (event->button() == Qt::MouseButton::RightButton) {
			move.mode = MoveState::MoveByMenu;
			move.moveToPos = event->position().toPoint();
			// note that the menu blocks the following actions
			move.menu.exec(event->globalPosition().toPoint());
			return;
		}
		cancelEvent = true;
	}

	markDrawing(clickedDrawing);

	if (!cancelEvent && event->button() == Qt::MouseButton::RightButton) {
		ctxMenu.setDrawingActionsVisible(clickedDrawing > 0);
		ctxMenu.menu.exec(event->globalPosition().toPoint());
		cancelEvent = true;
	}

	// forward to main window
	if (!cancelEvent) {
		const auto pos = event->position().toPoint();
		emit mouseClick(pos.x(), pos.y(), event->button(), drawings.getMarkedDrawingNum() > 0);
	}
}

void DrawArea::markDrawing(int drawingNum)
{
	if (drawings.setMarkedDrawing(drawingNum)) {
		update();
	}
}

void DrawArea::mouseReleaseEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	if (move.mode == MoveState::ReadyForMove || move.mode == MoveState::MoveStarted) {
		setCursor(Qt::ArrowCursor);
		if (move.mode == MoveState::MoveStarted && drawings.getHighlightedDrawingNum()) {
			emit highlightChanged(drawings.getHighlightedDrawResult());
		}
		move.mode = MoveState::NoMove;
	}
}

void DrawArea::mouseMoveEvent(QMouseEvent * event)
{
	if (move.mode == MoveState::ReadyForMove || move.mode == MoveState::MoveStarted) {
		if (move.mode == MoveState::ReadyForMove) {
			drawings.storeUndoPoint(drawings.getMarkedDrawingNum());
			setNextUndoRedo(true);
			move.mode = MoveState::MoveStarted;
		}
		const QPoint newOffset = move.startOffset + event->position().toPoint();
		if (drawings.moveDrawing(drawings.getMarkedDrawingNum(), newOffset, false)) update();
		setCursor(Qt::ClosedHandCursor);

	} else {
		const qint64 mouseOverDrawingNum = drawings.getDrawingByPos(event->pos());
		const bool moveCouldStart = (mouseOverDrawingNum > 0 && mouseOverDrawingNum == drawings.getMarkedDrawingNum());
		const bool readyForPaint = (drawings.getMarkedDrawingNum() == 0 && mouseOverDrawingNum == 0);
		setCursor(moveCouldStart ? Qt::OpenHandCursor : (readyForPaint ? Qt::CrossCursor : Qt::ArrowCursor));
		highlightDrawing(mouseOverDrawingNum);
	}
}

void DrawArea::highlightDrawing(int drawingNum)
{
	if (drawings.highlightDrawing(drawingNum)) {
		emit highlightChanged(drawings.getHighlightedDrawResult());
		update();
	}
}

void DrawArea::setNextUndoRedo(bool undoOrRedo)
{
	nextUndoOrRedo = undoOrRedo;
	ctxMenu.undoAction->setEnabled(undoOrRedo);
	ctxMenu.redoAction->setEnabled(!undoOrRedo);
}

void DrawArea::copyToClipboardMarked()
{
	bool ok;
	const bool transparent = ctxMenu.getTransparencyForExport(&ok);
	if (!ok) return;

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

void DrawArea::undoRedo()
{
	drawings.restoreLast();
	update();
	setNextUndoRedo(!nextUndoOrRedo);
}

void DrawArea::setBgColor()
{
	const QColor col = QColorDialog::getColor(drawings.backColor, this);
	if (col.isValid()) {

		drawings.backColor = col;
		drawings.redraw();
		update();
	}
}

// -------------------- ContextMenu --------------------

DrawArea::ContextMenu::ContextMenu(DrawArea * parent)
	: menu(parent)
	, drawArea(parent)
{
	drawingActions << menu.addAction("Delete drawing", Qt::Key_Delete, parent, &DrawArea::deleteMarked)
				   << menu.addAction("Copy drawing", Qt::CTRL | Qt::Key_C, parent, &DrawArea::copyToClipboardMarked)
				   << menu.addAction("Send to front", parent, &DrawArea::sendToFrontMarked)
				   << menu.addAction("Send to back", parent, &DrawArea::sendToBackMarked) << menu.addSeparator();

	setDrawingActionsVisible(false);

	undoAction = menu.addAction("Undo", Qt::CTRL | Qt::Key_Z, parent, &DrawArea::undoRedo);
	undoAction->setEnabled(false);
	redoAction = menu.addAction("Redo", Qt::CTRL | Qt::Key_Y, parent, &DrawArea::undoRedo);
	redoAction->setEnabled(false);
	menu.addSeparator();

	menu.addAction("Clear all", Qt::CTRL | Qt::Key_Delete, parent, &DrawArea::clear);
	menu.addAction("Set Bg-Color", Qt::CTRL | Qt::Key_B, parent, &DrawArea::setBgColor);
	menu.addSeparator();

	menu.addAction("Copy canvas", parent, &DrawArea::copyToClipboardFull);
	menu.addSeparator();

	menu.addAction("Show symbols window", Qt::CTRL | Qt::SHIFT | Qt::Key_S, parent, &DrawArea::emitShowSymbols);
}

void DrawArea::ContextMenu::setDrawingActionsVisible(bool visible)
{
	// shortcuts become enabled/disabled with making the actions (un)visible
	for (QAction * action : std::as_const(drawingActions)) {
		action->setVisible(visible);
	}
}

bool DrawArea::ContextMenu::getTransparencyForExport(bool * ok)
{
	bool transparent;
	if (transparencyForExport != TransparencyOpt::Ask) {
		transparent = (transparencyForExport == TransparencyOpt::Transparency);
	} else {
		bool doNotAskAnymore = false;
		// QMessageBox will take ownership (will delete the checkbox)
		QCheckBox * chkTransparency = new QCheckBox("Don't ask anymore until restart of lsystem");
		connect(chkTransparency, &QCheckBox::stateChanged, [&doNotAskAnymore](int state) { doNotAskAnymore = static_cast<bool>(state); });

		QMessageBox msgBox(QMessageBox::Icon::Question,
						   "Transparency",
						   "Do you want to export the drawing with transparent background (will not work in all programs)?",
						   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
						   drawArea->parentWidget());
		msgBox.setDefaultButton(QMessageBox::Yes);
		msgBox.setCheckBox(chkTransparency);
		QMessageBox::StandardButton res = static_cast<QMessageBox::StandardButton>(msgBox.exec());
		if (res == QMessageBox::Cancel) {
			*ok = false;
			return false;
		}
		transparent = (res == QMessageBox::Yes);
		if (doNotAskAnymore) {
			transparencyForExport = (transparent ? TransparencyOpt::Transparency : TransparencyOpt::NoTransparency);
		}
	}

	*ok = true;
	return transparent;
}

} // namespace lsystem::ui
