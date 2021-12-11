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

protected:
	void mousePressEvent(QMouseEvent * event);
};

#endif // CLICKABLELABEL_H
