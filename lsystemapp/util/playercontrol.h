#pragma once

#include <QQuickWidget>

class PlayerControl : public QQuickWidget
{
	Q_OBJECT
public:
    explicit PlayerControl(QWidget * parent);

private:
    QQuickItem * control_ = nullptr;
};
