#pragma once

#include <common.h>

#include <QAbstractListModel>
#include <QImage>
#include <QPainter>

#include <optional>

namespace lsystem::ui {

struct DrawingFrameSummary
{
	QPoint topLeft;
	QPoint botRight;
	QPoint offset;
	common::ConfigSet config;
};

struct DrawingSummary final : public DrawingFrameSummary
{
	qint64 drawingNum = 0;
	int listIndex = 0;
	int segmentsCount = 0;
	int animStep = 0;
};

class DrawingFrame
{
public:
	DrawingFrame(const common::ExecResult & execResult, const QSharedPointer<common::AllDrawData> & configAndMeta);
	DrawingFrameSummary toDrawingFrameSummary();

	QPoint offset;
	common::ConfigSet config;

protected:
	common::MetaData metaData;
	const bool paintLastIter;
	QPoint topLeft;
	QPoint botRight;

private:
	void expandSizeToSegments(const common::LineSegs & segs, double thickness);
	void updateRect(double minX, double minY, double maxX, double maxY);
};

class Drawing final : public DrawingFrame
{
public:
	Drawing(const common::ExecResult & execResult, const QSharedPointer<common::AllDrawData> & metaData);
	void drawToImage(QImage & dstImage, bool isMarked, bool isHighlighted);
	QPoint size() const;
	bool withinArea(const QPoint & pos);
	DrawingSummary toDrawingSummary();

	common::AnimatorResult newAnimationStep(int step, bool relativeStep);
	void drawBasicImage();

public:
	struct InternalMeta
	{
		double opacityFactor = 0;
		double thickness = 0;
		bool antiAliasing = false;
		std::optional<lsystem::common::ColorGradient> colorGradient;
	};

	qint64 num = 0;
	qint64 zIndex = 0;
	int listIndex = 0;
	common::LineSegs segments;
	QVector<QColor> actionColors;
	QImage lastIterImage;
	QImage image;
	InternalMeta mainMeta;
	std::optional<InternalMeta> lastIterMeta;
	bool usesOpacity = false;

	struct AnimState final
	{
		bool inProgress = false;
		int curSeg = 0; // index of the last segment which was painted
	} animState;

private:
	void drawSegments(const common::LineSegs & segs, const InternalMeta & meta);
	void drawSegmentRange(const common::LineSegs & segs, int numStart, int numEnd, const InternalMeta & meta);
};

// ------------------------------------------------------------------------------------

class DrawingCollection final : public QAbstractListModel
{
public:
	void addOrReplaceDrawing(const QSharedPointer<Drawing> & newDrawing);

	void resize(const QSize & newSize);
	void clearAll();

	void redraw(bool keepContent = false);

	void restoreLast();

	qint64 getDrawingByPos(const QPoint & pos);
	QPoint getDrawingOffset(qint64 drawingNum);
	QPoint getDrawingSize(qint64 drawingNum);
	QImage & getDrawingImage(qint64 drawingNum);
	QImage getImage();
	int getMarkedDrawingNum() const { return markedDrawing; }
	int getHighlightedDrawingNum() const { return highlightedDrawing; }
	Drawing * getCurrentDrawing();
	bool setMarkedDrawing(qint64 newMarkedDrawing);
	bool moveDrawing(qint64 drawingNum, const QPoint & newOffset, bool storeUndo = true);
	bool deleteDrawing(qint64 drawingNum);
	bool sendToFront(qint64 drawingNum);
	bool sendToBack(qint64 drawingNum);
	bool highlightDrawing(qint64 newHighlightedDrawing);
	std::optional<DrawingSummary> getHighlightedDrawResult();
	std::optional<DrawingSummary> getMarkedDrawResult();
	int getDrawingNumByListIndex(int listIndex) const;

	// called also externally, e.g., for move by drag
	void storeUndoPoint(std::optional<int> numToCopy = std::nullopt);

	// ListModel:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role) const override;

public:
	QColor backColor = QColor(255, 255, 255);
	bool dirty = false;

private:
	bool sendToZIndex(qint64 drawingNum, qint64 newZIndex);

	// ListModel:
	QString getRow(const QModelIndex & index) const;
	void allDataChanged();
	void updateListData();
	void updateZIndexToDrawing();

private:
	QImage image;

	QMap<qint64 /*drawNum*/, QSharedPointer<Drawing>> drawings;
	QMap<qint64 /*zIndex*/, qint64 /*drawNum*/> zIndexToDrawing;

	QMap<qint64 /*drawNum*/, QSharedPointer<Drawing>> lastDrawings;

	qint64 markedDrawing = 0;
	qint64 highlightedDrawing = 0;

	// Data for ListModel:
	struct ListEntry final
	{
		QString description;
		int drawNum = 0;
	};

	QVector<ListEntry> listData;
};

} // namespace lsystem::ui
