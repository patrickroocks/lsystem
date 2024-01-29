#pragma once

#include <QtCore>

namespace util {

template<typename T>
T ensureRange(T val, T lower, T upper)
{
	if (val < lower) return lower;
	else if (val > upper) return upper;
	return val;
}

} // namespace util
