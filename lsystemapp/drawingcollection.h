#ifndef DRAWINGCOLLECTION_H
#define DRAWINGCOLLECTION_H

#include <common.h>

#include <QImage>

namespace lsystem::ui {

struct DrawResult
{
	QPoint topLeft;
	QPoint botRight;
};

class Drawing
{
public:
	Drawing() = default;
	Drawing(const common::ExecResult & execResult, const QSharedPointer<common::MetaData> & metaData);
	void drawToImage(QImage & dstImage, bool isMarked, bool isHighlighted);
	QPoint size() const;
	bool withinArea(const QPoint & pos);
	bool move(const QPoint & newOffset);
	DrawResult toDrawResult();

public:
	qint64 zIndex = 0;
	qint64 num = 0;
	QPoint offset;
	qint64 numSegments = 0;
	QImage image;

private:
	void expandSizeToSegments(const common::LineSegs & segs, double thickness);
	void drawSegments(const common::LineSegs & segs, double opacyFactor, double thickness, bool antiAliasing);
	void updateRect(double minX, double minY, double maxX, double maxY);

private:
	QPoint topLeft;
	QPoint botRight;
};

class DrawingCollection
{
public:
	void addDrawing(Drawing newDrawing, const QPoint & off);

	void resize(const QSize & newSize);
	void clear();

	void redraw();
	QPoint getLastSize() const;

	qint64 getDrawingByPos(const QPoint & pos);
	QPoint getDrawingOffset(qint64 drawingNum);
	QPoint getDrawingSize(qint64 drawingNum);
	QImage & getDrawingImage(qint64 drawingNum);
	QImage getImage();
	int getMarkedDrawing() const { return markedDrawing; }
	int getHighlightedDrawing() const { return highlightedDrawing; }
	bool setMarkedDrawing(qint64 newMarkedDrawing);
	bool moveDrawing(qint64 drawingNum, const QPoint & newOffset);
	bool deleteImage(qint64 drawingNum);
	bool sendToFront(qint64 drawingNum);
	bool sendToBack(qint64 drawingNum);
	bool highlightDrawing(qint64 newHighlightedDrawing);
	std::optional<DrawResult> getHighlightedDrawResult();

public:
	QColor backColor = QColor(255, 255, 255);
	bool dirty = false;

private:
	bool sendToZIndex(qint64 drawingNum, qint64 newZIndex);

private:
	QImage image;
	qint64 lastDrawingNum = 0;

	QMap<qint64 /*drawNum*/, Drawing> drawings;
	QMap<qint64 /*zIndex*/, qint64 /*drawNum*/> zIndexToDrawing;

	qint64 markedDrawing = 0;
	qint64 highlightedDrawing = 0;
};

}

#endif // DRAWINGCOLLECTION_H
