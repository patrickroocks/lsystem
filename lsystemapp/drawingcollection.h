#pragma once

#include <common.h>

#include <QImage>
#include <QPainter>

#include <optional>

namespace lsystem::ui {

struct DrawResult
{
	QPoint topLeft;
	QPoint botRight;
	QPoint offset;
	qint64 drawingNum = 0;
	int segmentsCount = 0;
	int animStep = 0;
	lsystem::common::ConfigSet config;
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

	common::AnimatorResult newAnimationStep(int step, bool relativeStep);

public:
	qint64 zIndex = 0;
	qint64 num = 0;
	QPoint offset;
	common::LineSegs segments;
	QVector<QColor> actionColors;
	QImage image;
	lsystem::common::ConfigSet config;
	common::MetaData metaData;

	struct AnimState
	{
		bool inProgress = false;
		int curSeg = 0; // index of the last segment which was painted
	} animState;

private:
	void expandSizeToSegments(const common::LineSegs & segs, double thickness);
	void drawSegments(const common::LineSegs & segs, double opacyFactor, double thickness, bool antiAliasing);
	void updateRect(double minX, double minY, double maxX, double maxY);
	void drawSegmentRange(const common::LineSegs & segs, int numStart, int numEnd, double opacyFactor, double thickness, bool antiAliasing);

private:
	QPoint topLeft;
	QPoint botRight;
};

class DrawingCollection
{
public:
	void addDrawing(Drawing newDrawing, const QPoint & off);

	void resize(const QSize & newSize);
	void clearAll();
	void deleteHighlightedOrLastDrawing();

	void redraw(bool keepContent = false);
	QPoint getLastSize() const;

	qint64 getDrawingByPos(const QPoint & pos);
	QPoint getDrawingOffset(qint64 drawingNum);
	QPoint getDrawingSize(qint64 drawingNum);
	QImage & getDrawingImage(qint64 drawingNum);
	QImage getImage();
	int getMarkedDrawingNum() const { return markedDrawing; }
	int getHighlightedDrawingNum() const { return highlightedDrawing; }
	Drawing* getCurrentDrawing();
	int getLastDrawingNum() const { return drawings.empty() ? 0 : drawings.lastKey(); }
	bool setMarkedDrawing(qint64 newMarkedDrawing);
	bool moveDrawing(qint64 drawingNum, const QPoint & newOffset);
	bool deleteImage(qint64 drawingNum);
	bool sendToFront(qint64 drawingNum);
	bool sendToBack(qint64 drawingNum);
	bool highlightDrawing(qint64 newHighlightedDrawing);
	std::optional<DrawResult> getHighlightedDrawResult();
	std::optional<DrawResult> getMarkedDrawResult();
	std::optional<QPoint> getLastOffset() const;

public:
	QColor backColor = QColor(255, 255, 255);
	bool dirty = false;

private:
	bool sendToZIndex(qint64 drawingNum, qint64 newZIndex);

private:
	QImage image;

	QMap<qint64 /*drawNum*/, Drawing> drawings;
	QMap<qint64 /*zIndex*/, qint64 /*drawNum*/> zIndexToDrawing;

	qint64 markedDrawing = 0;
	qint64 highlightedDrawing = 0;
};

}
