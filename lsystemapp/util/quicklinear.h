#ifndef QUICKLINEAR_H
#define QUICKLINEAR_H

#include "quickbase.h"

class QuickLinear : public QuickBase
{
	Q_OBJECT
public:
	explicit QuickLinear(QWidget * parent = nullptr);

	void setExtensionFactor(double extFactor);

private:

	static const int sizeWidth = 180;
	static const int sizeHeight = 65;
	static const char * qmlSource;
};

#endif // QUICKLINEAR_H
