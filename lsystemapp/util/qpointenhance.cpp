#include "qpointenhance.h"

bool operator<=(const QPoint & lhs, const QPoint & rhs) { return lhs.x() <= rhs.x() && lhs.y() <= rhs.y(); }

bool operator>=(const QPoint & lhs, const QPoint & rhs) { return lhs.x() >= rhs.x() && lhs.y() >= rhs.y(); }

QLine operator-(const QLine & lhs, const QPoint & rhs)
{
	return QLine(lhs.x1() - rhs.x(), lhs.y1() - rhs.y(), lhs.x2() - rhs.x(), lhs.y2() - rhs.y());
}
