#include "watcherhandler.h"

#include <qsigwatcher/qsigwatcher.h>

#include <QTest>

namespace sigwatcher::impl {

SigWatcherHandler & SigWatcherHandler::instance()
{
	static SigWatcherHandler handler;
	return handler;
}

void SigWatcherHandler::regWatcher(Watcher * watcher)
{
	watchers << watcher;
}

void SigWatcherHandler::unregWatcher(Watcher * watcher)
{
	watchers.remove(watcher);
	if (watchers.isEmpty()) {
		// reached end of test, clean up state
		failed = false;
	}
}

void SigWatcherHandler::finallyFail(const Location & location, const QString & msg)
{
	const QString failMessage = util::printStr("signal failed at %1 with:\n", QDateTime::currentDateTimeUtc()) + msg;

	fail(failMessage, location.getFile().toUtf8().constData(), location.getLine());
}

void SigWatcherHandler::checkRelocate(const char * file, int line)
{
	checkLocation = Location(file, line);
	checkLocationPassed = false;
}

void SigWatcherHandler::reset()
{
	alreadyExited = false;
	failCounter = 0;
	checkLocationPassed = false;
}

bool SigWatcherHandler::check(qint64 timeoutMs, const char * file, int line, bool endOfTest)
{
	checkLocation = Location(file, line);
	checkLocationPassed = false;

	if (alreadyExited) return false;

	if (failed) {
		if (failCounter > 1) {
			const QString msg = QString("%1 more fails").arg(failCounter - 1);
			QTest::qFail(msg.toUtf8().constData(), __FILE__, __LINE__);
		}

		alreadyExited = true;
		checkLocationPassed = true;
		return false;
	}

	const QDateTime startWaitingTime = QDateTime::currentDateTimeUtc();
	QElapsedTimer timer;
	timer.start();

	WatcherState watcherState;
	while (!timer.hasExpired(timeoutMs)) {
		watcherState = checkWatchers();

		const bool stillWaiting = watcherState.someoneIsWaitingForExpected
				|| (!endOfTest && watcherState.someoneIsWaitingForUnexpected);

		if (!stillWaiting) break;
		QThread::msleep(50);
	}

	if (!watcherState.waitingWatchers.isEmpty()) {
		QString msg = util::printStr("these calls are still expected (between %1 and %2):", startWaitingTime, QDateTime::currentDateTimeUtc());
		for (const WatcherState::WaitingWatcher & watcher : std::as_const(watcherState.waitingWatchers)) {
			msg += QString("\n   signal (%1):").arg(watcher.watcherName)
				+ watcher.location.getQtCreatorStr();
			for (const ExpectOutput & expecting : watcher.expectingSignals) {
				msg += "\n\n   waiting for signal(s):" + expecting.getStrWithLoc();
			}
		}

		msg += "\n\n  checking here:";
		fail(msg, file, line);

		alreadyExited = true;
		checkLocationPassed = true;
		return false;
	}

	resetWatchers();

	checkLocationPassed = true;
	return true;
}

void SigWatcherHandler::fail(const QString & msg, const char * file, int line)
{
	if (!failed) {
		QTest::qFail(msg.toUtf8().constData(), file, line);
		failed = true;
	}
	failCounter++;
}

SigWatcherHandler::WatcherState SigWatcherHandler::checkWatchers()
{
	WatcherState rv;

	for (const Watcher * watcher : std::as_const(watchers)) {
		const WaitingState waitingState = watcher->getWaitingState();
		if (waitingState.isWaitingForExpected) {
			rv.waitingWatchers << WatcherState::WaitingWatcher{waitingState.location, waitingState.waitingList, watcher->getName()};
			rv.someoneIsWaitingForExpected = true;
		}
		if (waitingState.isWaitingForUnexpected) rv.someoneIsWaitingForUnexpected = true;
	}

	return rv;
}

void SigWatcherHandler::resetWatchers()
{
	for (Watcher * watcher : std::as_const(watchers)) watcher->reset();
}

}
