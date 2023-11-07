#pragma once

#include <util/divutils.h>

// -------------------------- end user structs --------------------------

namespace sigwatcher {

enum class IgnoreFlags {
	None = 0,
	OnlyOnce = 1,
	KeepForAllChecks = 2,
	IgnoreIfNotMatched = 4
};
ENUM_OR(IgnoreFlags)

enum class ExpectFlags {
	None = 0,
	IgnoreDuplicated = 1
};
ENUM_OR(ExpectFlags)

enum class WatcherFlags {
	None = 0,
	OrderSensitive = 1,
	IgnoreUnexpected = 2,
	WaitForUnexpected = 4
};
ENUM_OR(WatcherFlags)

}
