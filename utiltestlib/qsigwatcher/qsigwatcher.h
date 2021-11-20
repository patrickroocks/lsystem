#pragma once

#include <qsigwatcher/impl/sigbasewatcher.h>
#include <qsigwatcher/structures.h>

#include <QTest>

// ========================= end user macros =========================

// ----- constants -----

namespace sigwatcher::impl {

/// * Wait time used for SIG_(SLEEP)_CHECK_WAIT
const int DefaultWaitTimeMs = 1000;

/// * show also the rough structure of an expectation in the test output
const bool ShowExtendedExpectation = false;

}

// ----- declaration of the watcher -----

#define SIG_WATCHER(name, ...) \
	auto name = sigwatcher::impl::makeSigWatcher(__FILE__, __LINE__, #name, __VA_ARGS__); \
	SignalWatcherInterface::addWatcher(&name);
// if you get an error in the line above, you forgot to inherit from "SignalWatcherInterface"

// ----- expect & ignore -----

#define SIG_EXPECT(watcherName, ...) { \
		auto & __signal_watcher = watcherName; \
		watcherName.expect(__FILE__, __LINE__, __VA_ARGS__); \
	}

#define SIG_SUB_EXPECT(watcherName, ...) [&]{ \
		auto & __signal_watcher = watcherName; \
		return __VA_ARGS__; \
	}()

#define SIG_IGNORE(watcherName, ...) { \
	auto & __signal_watcher = watcherName; \
	watcherName.ignore(__FILE__, __LINE__, __VA_ARGS__); \
}


// * first argument of "..._AT" has "position=1"
// * all the expected functions/values need "..." / __VA_VARGS__ such that initializer list are accepted

#define CHECK_AT(position, ...) __signal_watcher.makeExpectCheckAt<position>(__VA_ARGS__)
#define VALUE_AT(position, ...) __signal_watcher.makeExpectValue<position>(__VA_ARGS__)

#define VALUE_CASTED_AT(position, CastToType, ...) __signal_watcher.makeExpectCastValue<position,CastToType>(#CastToType, __VA_ARGS__)
#define CHECK_CASTED_AT(position, CastToType, ...) __signal_watcher.makeExpectCastCheck<position,CastToType>(#CastToType, __VA_ARGS__)
#define VERIFY_CASTED_AT(position, CastToType) __signal_watcher.makeExpectCastVerify<position,CastToType>(#CastToType)

#define CHECK_RETURN return util::test::impl::TestResult(true);
#define CHECK_SUB(checkfun, ...) { \
		const util::test::impl::TestResult __sub_res = checkfun(__VA_ARGS__); \
		if (!__sub_res.success) return __sub_res; \
	}

#define VALUES(...) __signal_watcher.makeExpectValues(__VA_ARGS__)
#define CHECK(...) __signal_watcher.makeExpectCheck(__VA_ARGS__)
#define OCCUR __signal_watcher.makeExpectOccurrence()

// ----- prepare -----

#define SIG_PREPARE(watcherName, ...) { \
	auto & __signal_watcher_prepare = watcherName; \
	watcherName.setPrepare(__VA_ARGS__); \
}

#define PREPARE(...) __signal_watcher_prepare.makePrepare(__VA_ARGS__)
#define PREPARE_AT(position, ...) __signal_watcher_prepare.makePrepareAt<position>(__VA_ARGS__)
#define NO_PREPARE __signal_watcher_prepare.makeNoPrepare()

// ---- check & wait -----

#define SIG_CHECK \
	if (!sigwatcher::impl::SigWatcherHandler::instance().check(sigwatcher::impl::DefaultWaitTimeMs, __FILE__, __LINE__)) return;

#define SIG_CHECK_WAIT(timeoutMs) \
	if (!sigwatcher::impl::SigWatcherHandler::instance().check(timeoutMs, __FILE__, __LINE__)) return;

#define SIG_SLEEP_CHECK(sleepMs) \
	sigwatcher::impl::SigWatcherHandler::instance().checkRelocate(__FILE__, __LINE__); \
	QThread::msleep(sleepMs); \
	if (!sigwatcher::impl::SigWatcherHandler::instance().check(sigwatcher::impl::DefaultWaitTimeMs, __FILE__, __LINE__)) return;\

#define SIG_SLEEP_CHECK_WAIT(sleepMs, timeoutMs) \
	sigwatcher::impl::SigWatcherHandler::instance().checkRelocate(__FILE__, __LINE__); \
	QThread::msleep(sleepMs); \
	if (!sigwatcher::impl::SigWatcherHandler::instance().check(timeoutMs, __FILE__, __LINE__)) return;\


// -------------------------- watcher interface for your test --------------------------

namespace sigwatcher {

class SignalWatcherInterface
{
public:
	SignalWatcherInterface();
	~SignalWatcherInterface();

	void addWatcher(impl::Watcher * watcher);

private:
	QThread watcherThread;
};

// -------------------------- connect lambdas --------------------------

template<typename T>
inline std::function<QString(T)> operator&(std::function<QString(T)> func1, std::function<QString(T)> func2) {
	return [&](T val) {
		const QString res = func1(val);
		if (!res.isEmpty()) return res;
		return func2(val);
	 };
}

}

// -------------------------- implementation of Qt Signal/Slot Watcher --------------------------

namespace sigwatcher::impl {

template<class Obj, typename Ret, typename... Args>
class SigWatcher final : public SigBaseWatcher<Args...>
{
public:
	SigWatcher(const char * file, int line, const char * varName, Obj * sender, Ret (Obj::*signal) (Args...), const QFlags<WatcherFlags> & flags)
		: SigBaseWatcher<Args...>(file, line, varName, flags)
		, senderObj(sender)
		, signal(signal)
	{
		if (flags.testFlag(WatcherFlags::WaitForUnexpected) && flags.testFlag(WatcherFlags::IgnoreUnexpected)) {
			const QString msg = QString("Watcher flags \"WaitForUnexpected\" and \"IgnoreUnexpected\" must not be used at the same time");
			QTest::qFail(msg.toUtf8().constData(), file, line);
		}

		this->connect(sender, signal, this, &SigWatcher::recSignal);
	}

	~SigWatcher()
	{
		SigWatcherHandler::instance().check(DefaultWaitTimeMs, "end of test", 0, true);
		SigWatcherHandler::instance().unregWatcher(this);
		this->startDestroy();
	}

	void destroy() override
	{
		this->disconnect(senderObj, signal, this, &SigWatcher::recSignal);
	}

	void unreg()
	{
		this->disconnect(senderObj, signal, this, &SigWatcher::recSignal);
	}

private slots:
	void recSignal(Args... f)
	{
		this->sigReceived(f...);
	}

private:
	Obj * senderObj;
	Ret (Obj::*signal) (Args...);
};


template<class Obj, typename Ret, typename... Args>
inline SigWatcher<Obj, Ret, Args...> makeSigWatcher(const char * file, int line,
		const char * varName,
		Obj * sender, Ret (Obj::*signal) (Args...),
		const QFlags<WatcherFlags> & flags = WatcherFlags::None)
{
	return SigWatcher<Obj, Ret, Args...>(file, line, varName, sender, signal, flags);
}

}
