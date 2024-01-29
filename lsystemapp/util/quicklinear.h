#pragma once

#include "quickbase.h"

class QuickLinear final : public QuickBase
{
	Q_OBJECT
public:
	explicit QuickLinear(QWidget * parent = nullptr);

	void setExtensionFactor(double extFactor);
	void setFineStepSize(double fineStepSize);

protected:
	double fineStepSize() const override { return fineStepSize_; }

private:
	double fineStepSize_ = 0;

	static const int sizeWidth = 180;
	static const int sizeHeight = 65;
	static const char * qmlSource;
};
