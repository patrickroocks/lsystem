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
	void draw(const Drawing & drawing, int offX, int offY, bool clearBefore);
	void restoreLastImage();
	void copyToClipboardFull();

	void copyToClipboardMarked(bool transparent);
	void deleteMarked();
	void sendToFrontMarked();
	void sendToBackMarked();

	QPoint getLastSize() const;
	void setBgColor(const QColor & col);
	QColor getBgColor() const;

signals:
	void markingChanged(bool drawingMarked);
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
	QPoint moveStartOffset;
	qint64 markedDrawing = 0;
};

}

#endif // DRAWAREA_H
