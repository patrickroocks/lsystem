#include "quickangle.h"

namespace {

const char * qmlSource = "qrc:/util/angleselector.qml";

}

QuickAngle::QuickAngle(QWidget * parent)
	: QuickBase(parent, qmlSource)
{
	selector()->setProperty("size", size);
	setSizeXY(size, size);
	setMinMaxValue(-180, 180);
	setSmallBigStep(1, 5);
	placeAt(0, 0);
}

void QuickAngle::setValueRestriction(ValueRestriction valueRestriction)
{
	const int valueFilter = [&valueRestriction]() {
			switch (valueRestriction) {
			case ValueRestriction::None:
			case ValueRestriction::Numbers:
				return 0;
			case ValueRestriction::NegativeNumbers:
				return -1;
			case ValueRestriction::PositiveNumbers:
				return 1;
			default:
				return 0;
			}
		}();

	selector()->setProperty("valueFilter", valueFilter);
}

