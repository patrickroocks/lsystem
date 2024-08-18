#include "gradientpreview.h"

#include <QPaintEvent>
#include <QPainter>

using namespace lsystem::common;

namespace lsystem::ui {

GradientPreview::GradientPreview(QWidget * parent)
	: QWidget(parent)
{}

void GradientPreview::resizeEvent(QResizeEvent * event)
{
	QWidget::resizeEvent(event);
	updateGradient();
}

void GradientPreview::updateGradient()
{
	const int width = this->width();
	const int height = this->height();
	const double invWidth = 1.0 / width;

	gradientImage = QImage(width, height, QImage::Format_RGB32);
	QPainter painter(&gradientImage);
	QPen pen;
	pen.setWidth(1);

	// Paint a horizontal gradient
	for (int x = 0; x < width; ++x) {
		const double t = x * invWidth;
		pen.setColor(colorGradient->colorAt(t));
		painter.setPen(pen); // needed after setColor
		painter.drawLine(QPoint(x, 0), QPoint(x, height));
	}

	pen.setColor(QColor(0, 0, 0));
	painter.setPen(pen);
	painter.drawRect(0, 0, width - 1, height - 1);

	update();
}

void GradientPreview::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	QRect dirtyRect = event->rect();
	painter.drawImage(dirtyRect, gradientImage, dirtyRect);
}

} // namespace lsystem::ui
