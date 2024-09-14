#pragma once

#include <common.h>
#include <drawingcollection.h>

#include <QWidget>

namespace lsystem::ui {

class DrawArea : public QWidget
{
	Q_OBJECT
public:
	explicit DrawArea(QWidget * parent = nullptr);

	void clear();
	void draw(const Drawing & drawing);
	void restoreLastImage();
	void copyToClipboardFull();

	void copyToClipboardMarked(bool transparent);
	void deleteMarked();
	void deleteIndex(int index);
	void sendToFrontMarked();
	void sendToBackMarked();
	void translateHighlighted(const QPoint & newOffset);
	void markHighlighted();
	Drawing * getCurrentDrawing();

	void redrawAndUpdate(bool keepContent = false);

	void setBgColor(const QColor & col);
	QColor getBgColor() const;

	std::optional<DrawResult> getMarkedDrawingResult();

	DrawingCollection & getDrawingCollection() { return drawings; }

	void setIgnoreSelectionChange(bool ignore) { ignoreSelectionChange = ignore; }

public slots:
	common::AnimatorResult newAnimationStep(int step, bool relativeStep);
	void layerSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);

signals:
	void markingChanged();
	void highlightChanged(std::optional<DrawResult>);
	void mouseClick(int x, int y, Qt::MouseButton button, bool drawingMarked);
	void enableUndoRedo(bool undoOrRedo);

protected:
	void paintEvent(QPaintEvent * event) override;
	void resizeEvent(QResizeEvent * event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;

private:
	void clearAllDrawings();
	void setNextUndoRedo(bool undoOrRedo);
	void markDrawing(int drawingNum);

private:
	DrawingCollection drawings;

	bool nextUndoOrRedo = true;

	enum class MoveState
	{
		NoMove,
		ReadyForMove,
		MoveStarted
	};

	struct MoveInfo
	{
		MoveState mode = MoveState::NoMove;
		QPoint start;
		QPoint startOffset;
	} move;

	bool ignoreSelectionChange = false;
};

} // namespace lsystem::ui
