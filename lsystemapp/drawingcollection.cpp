#include "drawingcollection.h"

using namespace lsystem::common;

namespace lsystem::ui {

void DrawingCollection::addOrReplaceDrawing(const QSharedPointer<Drawing> & newDrawing)
{
	if (newDrawing->num == 0) {
		// add new drawing
		storeUndoPoint();
		newDrawing->zIndex = zIndexToDrawing.isEmpty() ? 1 : (zIndexToDrawing.lastKey() + 1);
		newDrawing->num = drawings.isEmpty() ? 1 : (drawings.lastKey() + 1);
		zIndexToDrawing[newDrawing->zIndex] = newDrawing->num;
	} else {
		// replace drawing, keep zIndex
		storeUndoPoint(newDrawing->num);
		newDrawing->zIndex = drawings[newDrawing->num]->zIndex;
	}

	drawings[newDrawing->num] = newDrawing;

	if (newDrawing->num == 0) {
		newDrawing->drawToImage(image, false, false);
	} else {
		redraw();
	}

	setMarkedDrawing(newDrawing->num);

	updateListData();
}

void DrawingCollection::storeUndoPoint(std::optional<int> numToCopy)
{
	// very simple 1-step undo
	lastDrawings = drawings;

	if (numToCopy.has_value()) {
		lastDrawings[numToCopy.value()] = QSharedPointer<Drawing>::create(*drawings[numToCopy.value()]);
	}
}

void DrawingCollection::restoreLast()
{
	qSwap(drawings, lastDrawings);

	updateZIndexToDrawing();
	if (markedDrawing > 0 && !drawings.contains(markedDrawing)) {
		// Remove marking. Calls redraw.
		setMarkedDrawing(0);
	} else {
		emit markingChanged();
		redraw();
	}
	updateListData();
}

void DrawingCollection::resize(const QSize & newSize)
{
	QImage newImage(newSize, QImage::Format_RGB32);
	newImage.fill(backColor);
	QPainter painter(&newImage);
	painter.drawImage(QPoint(0, 0), image);
	image = newImage;
}

void DrawingCollection::clearAll()
{
	if (drawings.isEmpty()) return;

	storeUndoPoint();

	markedDrawing = 0;
	highlightedDrawing = 0;
	drawings.clear();
	zIndexToDrawing.clear();

	image.fill(backColor);
}

void DrawingCollection::redraw(bool keepContent)
{
	dirty = false;

	if (!keepContent) image.fill(backColor);

	for (qint64 drawNum : std::as_const(zIndexToDrawing)) {
		drawings[drawNum]->drawToImage(image, !keepContent && drawNum == markedDrawing, !keepContent && drawNum == highlightedDrawing);
	}
}

qint64 DrawingCollection::getDrawingByPos(const QPoint & pos)
{
	if (zIndexToDrawing.isEmpty()) return 0;
	auto it = zIndexToDrawing.end();
	while (true) {
		it--;
		const qint64 drawNum = it.value();
		if (drawings[drawNum]->withinArea(pos)) return drawNum;
		if (it == zIndexToDrawing.begin()) break;
	}

	return 0;
}

QPoint DrawingCollection::getDrawingOffset(qint64 drawingNum)
{
	if (!drawings.contains(drawingNum)) return QPoint();
	return drawings[drawingNum]->offset;
}

QPoint DrawingCollection::getDrawingSize(qint64 drawingNum)
{
	if (!drawings.contains(drawingNum)) return QPoint();
	return drawings[drawingNum]->size();
}

QImage & DrawingCollection::getDrawingImage(qint64 drawingNum) { return drawings[drawingNum]->image; }

QImage DrawingCollection::getImage() { return image; }

Drawing * DrawingCollection::getCurrentDrawing()
{
	if (markedDrawing > 0) return drawings[markedDrawing].get();
	if (drawings.isEmpty()) return nullptr;
	return drawings.last().get();
}

bool DrawingCollection::setMarkedDrawing(qint64 newMarkedDrawing)
{
	// After a delete operation it can happen that we try to mark an unexisting drawing
	if (!drawings.contains(newMarkedDrawing)) newMarkedDrawing = 0;

	if (markedDrawing == newMarkedDrawing) return false;

	markedDrawing = newMarkedDrawing;
	emit markingChanged();

	redraw();
	return true;
}

bool DrawingCollection::moveDrawing(qint64 drawingNum, const QPoint & newOffset, bool storeUndo)
{
	if (!drawings.contains(drawingNum) || drawings[drawingNum]->offset == newOffset) return false;

	if (storeUndo) storeUndoPoint(drawingNum);

	drawings[drawingNum]->offset = newOffset;
	redraw();
	updateListData();
	return true;
}

bool DrawingCollection::deleteDrawing(qint64 drawingNum)
{
	if (!drawings.contains(drawingNum)) return false;

	storeUndoPoint();
	zIndexToDrawing.remove(drawings[drawingNum]->zIndex);
	drawings.remove(drawingNum);
	if (markedDrawing == drawingNum) markedDrawing = 0;
	if (highlightedDrawing == drawingNum) highlightedDrawing = 0;
	updateListData();
	redraw();
	return true;
}

bool DrawingCollection::sendToFront(qint64 drawingNum)
{
	if (zIndexToDrawing.isEmpty()) return false;
	return sendToZIndex(drawingNum, zIndexToDrawing.lastKey() + 1);
}

bool DrawingCollection::sendToBack(qint64 drawingNum)
{
	if (zIndexToDrawing.isEmpty()) return false;
	return sendToZIndex(drawingNum, zIndexToDrawing.firstKey() - 1);
}

bool DrawingCollection::highlightDrawing(qint64 newHighlightedDrawing)
{
	if (highlightedDrawing == newHighlightedDrawing) return false;
	highlightedDrawing = newHighlightedDrawing;
	redraw();
	return true;
}

std::optional<DrawingSummary> DrawingCollection::getHighlightedDrawResult()
{
	if (!highlightedDrawing) {
		return {};
	} else {
		return drawings[highlightedDrawing]->toDrawingSummary();
	}
}

std::optional<DrawingSummary> DrawingCollection::getMarkedDrawResult()
{
	if (!markedDrawing) {
		return {};
	} else {
		return drawings[markedDrawing]->toDrawingSummary();
	}
}

bool DrawingCollection::sendToZIndex(qint64 drawingNum, qint64 newZIndex)
{
	if (!drawings.contains(drawingNum)) return false;
	auto & drawing = drawings[drawingNum];
	const qint64 oldZIndex = drawing->zIndex;
	if (oldZIndex == newZIndex) return false;

	storeUndoPoint(drawingNum);
	drawing->zIndex = newZIndex;
	updateZIndexToDrawing();
	redraw();
	updateListData();
	return true;
}

int DrawingCollection::rowCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);
	return drawings.size();
}

QVariant DrawingCollection::data(const QModelIndex & index, int role) const
{
	if (role == Qt::DisplayRole && index.column() == 0) {
		if (index.row() < drawings.size()) {
			return getRow(index);
		}
	}

	return QVariant();
}

QString DrawingCollection::getRow(const QModelIndex & index) const
{
	if (index.row() >= listData.size()) return QString{};
	return listData[index.row()].description;
}

void DrawingCollection::allDataChanged() { emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0)); }

void DrawingCollection::updateListData()
{
	listData.clear();
	for (qint64 drawNum : std::as_const(zIndexToDrawing)) {
		ListEntry entry;
		auto & drawing = drawings[drawNum];
		entry.description = "[" + QString::number(drawNum) + "] " + drawing->config.name + " (" + QString::number(drawing->offset.x())
							+ ", " + QString::number(drawing->offset.y()) + ")";
		entry.drawNum = drawNum;
		drawing->listIndex = listData.size();
		listData.push_back(entry);
	}

	allDataChanged();
}

int DrawingCollection::getDrawingNumByListIndex(int listIndex) const
{
	if (listIndex < 0 || listIndex >= listData.size()) return 0;
	return listData[listIndex].drawNum;
}

void DrawingCollection::updateZIndexToDrawing()
{
	zIndexToDrawing.clear();
	for (const auto & drawing : std::as_const(drawings)) {
		zIndexToDrawing[drawing->zIndex] = drawing->num;
	}
}

} // namespace lsystem::ui
