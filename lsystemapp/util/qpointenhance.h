#pragma once

#include <QtCore>

bool operator<=(const QPoint & lhs, const QPoint & rhs)
{
	return lhs.x() <= rhs.x() && lhs.y() <= rhs.y();
}

bool operator>=(const QPoint & lhs, const QPoint & rhs)
{
	return lhs.x() >= rhs.x() && lhs.y() >= rhs.y();
}
