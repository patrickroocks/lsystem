#include "quicklinear.h"


const char * QuickLinear::qmlSource = "qrc:/util/linearselector.qml";

QuickLinear::QuickLinear(QWidget * parent)
	: QuickBase(parent, qmlSource)
{
	setSizeXY(sizeWidth, sizeHeight);
	selector()->setProperty("sizeWidth", sizeWidth);
	selector()->setProperty("sizeHeight", sizeHeight);
	placeAt(0, 0);
}

void QuickLinear::setExtensionFactor(double extFactor)
{
	selector()->setProperty("extensionFactor", extFactor);
}

void QuickLinear::setFineStepSize(double fineStepSize)
{
	this->fineStepSize = fineStepSize;
	selector()->setProperty("fineStepSize", fineStepSize);
}

void QuickLinear::setValue(double newValue)
{
	const bool isFine = newValue != qRound(newValue) && fineStepSize > 0;
	selector()->setProperty("rangeStepSmall", isFine);
	selector()->setProperty("rangeStepFactor", isFine ? fineStepSize : 1);
	QuickBase::setValue(newValue);
}
