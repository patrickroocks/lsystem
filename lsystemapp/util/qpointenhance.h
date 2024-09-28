#pragma once

#include <QtCore>

bool operator<=(const QPoint & lhs, const QPoint & rhs);
bool operator>=(const QPoint & lhs, const QPoint & rhs);
QLine operator-(const QLine & lhs, const QPoint & rhs);
