#ifndef QUICKBASE_H
#define QUICKBASE_H

#include <QLineEdit>
#include <QQuickItem>
#include <QQuickWidget>

class QuickBase : public QQuickWidget
{
	Q_OBJECT
public:
	explicit QuickBase(QWidget * parent, const char * qmlSource);
	void placeAt(int x, int y);
	void setValue(int newValue);
	void setLineEdit(QLineEdit * newLineEdit);

	void setSmallBigStep(double newSmallStep, double newBigStep);
	void setMinMaxValue(double newMinValue, double newMaxValue);

signals:
	void valueChanged(int value);
	void focusOut();

protected:
	void focusOutEvent(QFocusEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
	void keyReleaseEvent(QKeyEvent * event) override;

	void setSizeXY(int newSizeX, int newSizeY);

	QQuickItem * selector() { return selector_; }
	double minValue() { return minValue_; }
	double maxValue() { return maxValue_; }
	double smallStep() { return smallStep_; }
	double bigStep() { return bigStep_; }


private slots:
	void valueChangedInSelector();

private:
	double currentValue = 0;

	QQuickItem * selector_ = nullptr;
	QLineEdit * lineEdit = nullptr;

	int sizeX = 0;
	int sizeY = 0;

	double smallStep_ = 0;
	double bigStep_ = 0;
	double minValue_ = 0;
	double maxValue_ = 0;
};

#endif // QUICKBASE_H
