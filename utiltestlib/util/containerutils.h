#pragma once

#include <vector>

namespace util {

template <typename T>
std::vector<T> concatenateVectors(const std::vector<T>& lhs, const std::vector<T>& rhs)
{
	std::vector<T> rv;
	rv.reserve(lhs.size() + rhs.size());
	rv.insert(rv.end(), lhs.begin(), lhs.end());
	rv.insert(rv.end(), rhs.begin(), rhs.end());
	return rv;
}

}
