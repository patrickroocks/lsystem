#include "qsigwatcher.h"
#include <qsigwatcher/impl/internalstructures.h>

namespace sigwatcher {

using namespace impl;

SignalWatcherInterface::SignalWatcherInterface()
{
	ExpectOutput::showExtendedExpectation = ShowExtendedExpectation;
	watcherThread.start();
}

SignalWatcherInterface::~SignalWatcherInterface()
{
	util::quitAndWait({&watcherThread});
}

void SignalWatcherInterface::addWatcher(Watcher * watcher)
{
	watcher->moveToThread(&watcherThread);
	SigWatcherHandler::instance().regWatcher(watcher);
}

}
