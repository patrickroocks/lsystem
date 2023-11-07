#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

class ClickableLabel : public QLabel
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

#endif // CLICKABLELABEL_H
