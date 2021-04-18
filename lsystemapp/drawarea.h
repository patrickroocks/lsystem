#ifndef DRAWAREA_H
#define DRAWAREA_H

#include <QWidget>

#include <common.h>

class DrawArea : public QWidget
{
	Q_OBJECT
public:
	explicit DrawArea(QWidget *parent = nullptr);

	void clear();
	void draw(const lsystem::common::LineSegs & segs, int offX, int offY, bool clear);
	void restoreLastImage();
	void copyToClipboardFull();
	void copyToClipboardLastDrawing();
	QPoint getLastSize() const;

public:
	QRgb backColor = qRgb(255, 255, 255);

signals:
	void mouseClick(int x, int y, Qt::MouseButton button);
	void enableUndoRedu(bool undoOrRedo);

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

private:
	void resizeImage(QImage *image, const QSize &newSize);
	void setNextUndoRedo(bool undoOrRedo);

private:
	struct Drawing {
		bool rectValid = false;
		QPoint topLeft;
		QPoint botRight;
		QImage image;
		void updateRect(double minX, double minY, double maxX, double maxY);
	};

	Drawing drawing;
	Drawing lastDrawing;

	bool nextUndoOrRedo = true;
};

#endif // DRAWAREA_H
