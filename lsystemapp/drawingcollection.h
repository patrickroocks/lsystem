#pragma once

#include <drawing.h>

#include <QAbstractListModel>

namespace lsystem::ui {

class DrawingCollection final : public QAbstractListModel
{
	Q_OBJECT
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

signals:
	void markingChanged();

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
