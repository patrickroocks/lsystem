#include "drawarea.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPointF>
#include <QRectF>
#include <QClipboard>
#include <QGuiApplication>

using namespace lsystem::common;

DrawArea::DrawArea(QWidget * parent) : QWidget(parent)
{
}

void DrawArea::clear()
{
	lastImage = image;
	image.fill(backColor);
	update();
	setNextUndoRedo(true);
}

void DrawArea::draw(const LineSegs & segs, int offX, int offY, bool clear)
{
	lastImage = image;

	if (clear) image.fill(backColor);

	QPen pen;
	QPainter painter(&image);

	QPointF off(offX, offY);

	drawing.rectValid = false;
	drawing.topLeft = QPointF(image.width(), image.height());
	drawing.botRight = QPointF(0, 0);

	for (const LineSeg & seg : segs) {
		pen.setColor(seg.color);
		painter.setPen(pen);
		const QPointF start = seg.start + off;
		const QPointF end   = seg.end   + off;
		painter.drawLine(start, end);

		drawing.updateRect(qMin(start.x(), end.x()), qMin(start.y(), end.y()),
						   qMax(start.x(), end.x()), qMax(start.y(), end.y()));
	}

	if (!segs.isEmpty()) drawing.rectValid = true;

	update();

	setNextUndoRedo(true);
}

void DrawArea::restoreLastImage()
{
	QImage tmpImage = image;
	image = lastImage;
	lastImage = tmpImage;

	update();

	setNextUndoRedo(!nextUndoOrRedo);
}

void DrawArea::copyToClipboardFull()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setImage(image);
}

void DrawArea::copyToClipboardLastDrawing()
{
	if (!drawing.rectValid) return;

	QClipboard * clipboard = QGuiApplication::clipboard();
	const QPoint pt = QPointF(drawing.botRight - drawing.topLeft).toPoint();
	QImage newImage(QSize(pt.x(), pt.y()), QImage::Format_RGB32);
	QPainter painter(&newImage);
	painter.drawImage(QPoint(0, 0), image, QRectF(drawing.topLeft, drawing.botRight));
	clipboard->setImage(newImage);
}

QPoint DrawArea::getLastSize() const
{
	if (!drawing.rectValid) return QPoint();
	return (drawing.botRight - drawing.topLeft).toPoint();
}

void DrawArea::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	QRect dirtyRect = event->rect();
	painter.drawImage(dirtyRect, image, dirtyRect);
}

void DrawArea::resizeEvent(QResizeEvent * event)
{
	if (width() > image.width() || height() > image.height()) {
		// avoid always resizing, wehen the window size is changed
		int newWidth = qMax(width() + 128, image.width());
		int newHeight = qMax(height() + 128, image.height());
		resizeImage(&image, QSize(newWidth, newHeight));
		update();
	}
	QWidget::resizeEvent(event);
}

void DrawArea::mousePressEvent(QMouseEvent * event)
{
	emit mouseClick(event->x(), event->y(), event->button());
}

void DrawArea::resizeImage(QImage * image, const QSize & newSize)
{
	if (image->size() == newSize)
		return;

	QImage newImage(newSize, QImage::Format_RGB32);
	newImage.fill(backColor);
	QPainter painter(&newImage);
	painter.drawImage(QPoint(0, 0), *image);
	*image = newImage;

	newImage.fill(backColor);
	painter.drawImage(QPoint(0, 0), lastImage);
	lastImage = newImage;
}

void DrawArea::setNextUndoRedo(bool undoOrRedo)
{
	nextUndoOrRedo = undoOrRedo;
	emit enableUndoRedu(nextUndoOrRedo);
}

// ------------------------------------------------------------------------------------------

void DrawArea::Drawing::updateRect(double minX, double minY, double maxX, double maxY)
{
	if (minX < topLeft .x()) topLeft .setX(minX);
	if (minY < topLeft .y()) topLeft .setY(minY);
	if (maxX > botRight.x()) botRight.setX(maxX);
	if (maxY > botRight.y()) botRight.setY(maxY);
}
