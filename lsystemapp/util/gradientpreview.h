#pragma once

#include <common.h>

#include <QImage>
#include <QWidget>

namespace lsystem::ui {

class GradientPreview : public QWidget
{
public:
	explicit GradientPreview(QWidget * parent);
	void updateGradient();
	void setColorGradient(lsystem::common::ColorGradient * newColorGradient) { colorGradient = newColorGradient; }

protected:
	void paintEvent(QPaintEvent * event) override;
	void resizeEvent(QResizeEvent * event) override;

private:
	lsystem::common::ColorGradient * colorGradient = nullptr;
	QImage gradientImage;
};

} // namespace lsystem::ui
