#ifndef QUICKLINEAR_H
#define QUICKLINEAR_H

#include <QQuickWidget>
#include <QLineEdit>

class QuickLinear : public QQuickWidget
{
	Q_OBJECT
public:
	explicit QuickLinear(QWidget * parent = nullptr);

	void placeAt(int x, int y);
	void setExtensionFactor(double extFactor);
	void setValue(double newValue);
	void setMinMaxValue(double newMinValue, double newMaxValue);
	void setSmallBigStep(double newSmallStep, double newBigStep);
	void setLineEdit(QLineEdit * newLineEdit);

signals:
	void valueChanged(double value);
	void focusOut();

protected:
	void focusOutEvent(QFocusEvent * event) override;
	void keyPressEvent(QKeyEvent * event) override;
	void keyReleaseEvent(QKeyEvent * event) override;

private slots:
	void valueChangedInSelector();


private:
	double currentValue = 0;
	double smallStep = 1;
	double bigStep = 3;
	double minValue = 1;
	double maxValue = 20;

	static const int sizeWidth = 150;
	static const int sizeHeight = 65;
	static const char * qmlSource;

	QQuickItem * selector;
	QLineEdit * lineEdit = nullptr;
};

#endif // QUICKLINEAR_H
