#ifndef DRAWINGCOLLECTION_H
#define DRAWINGCOLLECTION_H

#include <common.h>

#include <QImage>

namespace lsystem::ui {

class Drawing
{
public:
	static Drawing fromSegments(const common::LineSegs & segs);
	void drawToImage(QImage & dstImage, bool isMarked);
	QPoint size() const;
	bool withinArea(const QPoint & pos);
	void move(const QPoint & newOffset);
public:
	qint64 zIndex = 0;
	qint64 num = 0;
	QPoint offset;
	QImage image;
private:
	void updateRect(double minX, double minY, double maxX, double maxY);
private:
	QPoint topLeft;
	QPoint botRight;
};


class DrawingCollection
{
public:
	void addDrawing(const lsystem::common::LineSegs & segs, const QPoint & off);

	void resize(const QSize & newSize);
	void clear();

	void redraw();
	QPoint getLastSize() const;

	qint64 getDrawingByPos(const QPoint & pos);
	QPoint getDrawingOffset(qint64 drawingNum);
	QPoint getDrawingSize(qint64 drawingNum);
	QImage & getImage(qint64 drawingNum);
	bool setMarkedDrawing(qint64 newMarkedDrawing);
	bool moveDrawing(qint64 drawingNum, const QPoint & newOffset);
	bool deleteImage(qint64 drawingNum);
	bool sendToFront(qint64 drawingNum);
	bool sendToBack(qint64 drawingNum);

public:
	QImage image;
	QColor backColor = QColor(255, 255, 255);
	bool dirty = false;

private:
	bool sendToZIndex(qint64 drawingNum, qint64 newZIndex);

private:
	qint64 lastDrawingNum = 0;

	QMap<qint64 /*drawNum*/, Drawing> drawings;
	QMap<qint64 /*zIndex*/, qint64 /*drawNum*/> zIndexToDrawing;

	qint64 markedDrawing = 0;
};

}

#endif // DRAWINGCOLLECTION_H
