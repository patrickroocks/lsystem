#include "sigbasewatcher.h"

namespace sigwatcher::impl {

QMutex Watcher::mutex(QMutex::Recursive);

}
