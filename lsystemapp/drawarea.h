#ifndef DRAWAREA_H
#define DRAWAREA_H

#include <drawingcollection.h>
#include <common.h>

#include <QWidget>

namespace lsystem::ui {

class DrawArea : public QWidget
{
	Q_OBJECT
public:
	explicit DrawArea(QWidget * parent = nullptr);

	void clear();
	void draw(const Drawing & drawing, const QPoint & offset, bool clearAll, bool clearLast);
	void restoreLastImage();
	void copyToClipboardFull();

	void copyToClipboardMarked(bool transparent);
	void deleteMarked();
	void sendToFrontMarked();
	void sendToBackMarked();
	void translateHighlighted(const QPoint & newOffset);

	void setBgColor(const QColor & col);
	QColor getBgColor() const;

	std::optional<QPoint> getLastOffset() const;
	std::optional<DrawResult> getMarkedDrawingResult();

signals:
	void markingChanged(bool drawingMarked);
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
	void resizeImage(QImage * image, const QSize & newSize);
	void setNextUndoRedo(bool undoOrRedo);

private:
	DrawingCollection drawings;
	DrawingCollection lastDrawings;

	bool nextUndoOrRedo = true;

	enum MoveState {
		NoMove,
		ReadyForMove,
		MoveStarted
	};

	MoveState moveMode = MoveState::NoMove;
	QPoint moveStart;
	QPoint moveStartOffset;
};

}

#endif // DRAWAREA_H
