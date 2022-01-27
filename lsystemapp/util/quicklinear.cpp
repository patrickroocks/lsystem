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
