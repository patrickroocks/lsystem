#ifndef QUICKANGLE_H
#define QUICKANGLE_H

#include "valuerestriction.h"
#include "quickbase.h"

class QuickAngle final : public QuickBase
{
	Q_OBJECT
public:
	explicit QuickAngle(QWidget * parent = nullptr);
	void setValueRestriction(ValueRestriction valueRestriction);

protected:
	double fineStepSize() const override { return 0.1; }

private:
    static const int size = 180;
	static const int bigStep = 5;
};

#endif // QUICKANGLE_H
