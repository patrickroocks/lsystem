#ifndef QUICKANGLE_H
#define QUICKANGLE_H

#include "valuerestriction.h"

#include <QQuickWidget>
#include <QLineEdit>

class QuickAngle : public QQuickWidget
{
	Q_OBJECT
public:
	explicit QuickAngle(QWidget * parent = nullptr);

	void placeAt(int x, int y);
	void setValue(int newValue);
	void setLineEdit(QLineEdit * newLineEdit);
	void setValueRestriction(ValueRestriction valueRestriction);

signals:
	void valueChanged(int value);
	void focusOut();

protected:
	void focusOutEvent(QFocusEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
	void keyReleaseEvent(QKeyEvent * event) override;

private slots:
	void valueChangedInSelector();


private:
	int currentValue = 0;
	static const int size = 150;
	static const char * qmlSource;
	QQuickItem * selector = nullptr;
	QLineEdit * lineEdit = nullptr;
};

#endif // QUICKANGLE_H
