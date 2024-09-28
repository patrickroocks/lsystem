#pragma once

#include <common.h>
#include <drawingcollection.h>

#include <QMenu>
#include <QWidget>

namespace lsystem::ui {

class DrawArea : public QWidget
{
	Q_OBJECT
public:
	explicit DrawArea(QWidget * parent = nullptr);

	void clear();
	void draw(const QSharedPointer<ui::Drawing> & drawing);
	void copyToClipboardFull();

	void deleteMarked();
	void deleteIndex(int index);
	void sendToFrontMarked();
	void sendToBackMarked();
	void translateHighlighted(const QPoint & newOffset);
	void markHighlighted();
	void markDrawing(int drawingNum);
	Drawing * getCurrentDrawing();

	void redrawAndUpdate(bool keepContent = false);

	std::optional<DrawingSummary> getMarkedDrawingResult();

	DrawingCollection & getDrawingCollection() { return drawings; }

	void setIgnoreSelectionChange(bool ignore) { ignoreSelectionChange = ignore; }

	QMenu * getContextMenu() { return &ctxMenu.menu; }

public slots:
	common::AnimatorResult newAnimationStep(int step, bool relativeStep);
	void layerSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
	void moveDrawingHere();

signals:
	void highlightChanged(std::optional<DrawingSummary>);
	void mouseClick(int x, int y, Qt::MouseButton button, bool drawingMarked);
	void showSymbols();

protected:
	void paintEvent(QPaintEvent * event) override;
	void resizeEvent(QResizeEvent * event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;

private:
	void setNextUndoRedo(bool undoOrRedo);
	void highlightDrawing(int drawingNum);
	void deleteDrawing(int drawingNum);

private slots:
	void copyToClipboardMarked();
	void undoRedo();
	void setBgColor();
	void emitShowSymbols() { emit showSymbols(); }

private:
	DrawingCollection drawings;

	bool nextUndoOrRedo = true;

	enum class MoveState
	{
		NoMove,
		ReadyForMove,
		MoveStarted,
		MoveByMenu
	};

	enum class TransparencyOpt
	{
		Ask = 0,
		NoTransparency = 1,
		Transparency = 2
	};

	struct MoveInfo final
	{
		MoveState mode = MoveState::NoMove;
		QPoint moveToPos;
		QPoint startOffset;
		QMenu menu;
	} move;

	class ContextMenu final
	{
	public:
		ContextMenu(DrawArea * parent);
		QMenu menu;
		QAction * undoAction;
		QAction * redoAction;
		void setDrawingActionsVisible(bool visible);
		bool getTransparencyForExport(bool * ok);

	private:
		DrawArea * const drawArea;
		QList<QAction *> drawingActions;
		TransparencyOpt transparencyForExport = TransparencyOpt::Ask;
	} ctxMenu;

	bool ignoreSelectionChange = false;
};

} // namespace lsystem::ui
