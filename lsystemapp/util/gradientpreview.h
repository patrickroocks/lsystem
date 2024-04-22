#pragma once

#include <common.h>

#include <QImage>
#include <QWidget>

namespace lsystem::ui {

class GradientPreview : public QWidget
{
public:
	explicit GradientPreview(const lsystem::common::ColorGradient & colorGradient, QWidget * parent);
	void updateGradient();

protected:
	void paintEvent(QPaintEvent * event) override;
	void resizeEvent(QResizeEvent * event) override;

private:
	const lsystem::common::ColorGradient & colorGradient;
	QImage gradientImage;
};

} // namespace lsystem::ui
