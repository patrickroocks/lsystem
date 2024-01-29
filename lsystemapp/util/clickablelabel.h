#pragma once

#include <QLabel>

class ClickableLabel final : public QLabel
{
	Q_OBJECT

public:
	explicit ClickableLabel(QWidget * parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());

signals:
	void mousePressed(QMouseEvent * event);
	void mouseReleased(QMouseEvent * event);
	void mouseMoved(QMouseEvent * event);

protected:
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
};
