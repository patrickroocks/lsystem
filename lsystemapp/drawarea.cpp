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
	lastDrawing = drawing;

	drawing.image.fill(backColor);
	update();
	setNextUndoRedo(true);
}

void DrawArea::draw(const LineSegs & segs, int offX, int offY, bool clear)
{
	lastDrawing = drawing;

	QImage & image = drawing.image;
	if (clear) image.fill(backColor);

	QPen pen;
	QPainter painter(&image);

	QPointF off(offX, offY);

	drawing.rectValid = false;
	drawing.topLeft  = QPoint(image.width(), image.height());
	drawing.botRight = QPoint(0, 0);

	for (const LineSeg & seg : segs) {
		pen.setColor(seg.color);
		painter.setPen(pen);
		const QPoint start = (seg.start + off).toPoint();
		const QPoint end   = (seg.end   + off).toPoint();
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
	qSwap(drawing, lastDrawing);
	update();
	setNextUndoRedo(!nextUndoOrRedo);
}

void DrawArea::copyToClipboardFull()
{
	QClipboard * clipboard = QGuiApplication::clipboard();
	clipboard->setImage(drawing.image);
}

void DrawArea::copyToClipboardLastDrawing()
{
	if (!drawing.rectValid) return;

	QClipboard * clipboard = QGuiApplication::clipboard();
	const QPoint size = drawing.botRight - drawing.topLeft + QPoint(1, 1);
	QImage newImage(QSize(size.x(), size.y()), QImage::Format_RGB32);
	QPainter painter(&newImage);
	painter.drawImage(QPoint(0, 0), drawing.image, QRect(drawing.topLeft, drawing.botRight + QPoint(1, 1)));
	clipboard->setImage(newImage);
}

QPoint DrawArea::getLastSize() const
{
	if (!drawing.rectValid) return QPoint();
	return drawing.botRight - drawing.topLeft;
}

void DrawArea::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	QRect dirtyRect = event->rect();
	painter.drawImage(dirtyRect, drawing.image, dirtyRect);
}

void DrawArea::resizeEvent(QResizeEvent * event)
{
	QImage & image = drawing.image;
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
	if (image->size() == newSize) return;

	QImage newImage(newSize, QImage::Format_RGB32);
	newImage.fill(backColor);
	QPainter painter(&newImage);
	painter.drawImage(QPoint(0, 0), *image);
	*image = newImage;

	newImage.fill(backColor);
	painter.drawImage(QPoint(0, 0), lastDrawing.image);
	lastDrawing.image = newImage;
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
