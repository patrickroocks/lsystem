#pragma once

#include <qsigwatcher/impl/internalstructures.h>

namespace sigwatcher {
class SignalWatcherInterface;
}

namespace sigwatcher::impl {

template <typename... F> class SigBaseWatcher;
template <class Obj, typename Ret, typename... Args> class SigWatcher;
class Watcher;

class SigWatcherHandler final : public QObject
{
	Q_OBJECT
public:
	void reset();
	bool check(qint64 timeoutMs, const char * file, int line, bool endOfTest = false);
	void checkRelocate(const char * file, int line);

	static SigWatcherHandler & instance();

protected:

	template <typename... F>
	friend class SigBaseWatcher;
	template <class Obj, typename Ret, typename... Args>
	friend class SigWatcher;
	friend class ::sigwatcher::SignalWatcherInterface;

	// methods to call from watchers
	void regWatcher(Watcher * watcher);
	void unregWatcher(Watcher * watcher);
	void finallyFail(const Location & location, const QString & msg);

private:
	void fail(const QString & msg, const char * file, int line);

	struct WatcherState {
		bool someoneIsWaitingForExpected = false;
		bool someoneIsWaitingForUnexpected = false;

		struct WaitingWatcher {
			Location location;
			QList<ExpectOutput> expectingSignals;
			QString watcherName;
		};
		QList<WaitingWatcher> waitingWatchers;
	};

	WatcherState checkWatchers();
	void resetWatchers();

private:

	QSet<Watcher *> watchers;
	std::atomic<bool> failed = ATOMIC_VAR_INIT(false);
	std::atomic<quint32> failCounter = ATOMIC_VAR_INIT(0);

	bool alreadyExited = false;
	bool checkLocationPassed = false;
	Location checkLocation;
};

}
