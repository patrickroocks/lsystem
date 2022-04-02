#include <QtTest>

#include <simulator.h>

#include <qsigwatcher/qsigwatcher.h>
#include <util/print.h>
#include <util/test/utils.h>

using namespace lsystem::common;
using namespace lsystem;
using namespace sigwatcher;
using namespace util::test;
using namespace util;

namespace {

constexpr int StackSize = 1000;

}

class SimulatorBaseTest : public QObject, public SignalWatcherInterface
{
	Q_OBJECT

public:
	SimulatorBaseTest()
	{
		lsystem::common::registerCommonTypes();
	}

private slots:

	void init()
	{
		simulator.setMaxStackSize(StackSize);
		simulator.moveToThread(&simulatorThread);
		connect(this, &SimulatorBaseTest::execAndExpand, &simulator, &Simulator::execAndExpand);
		connect(this, &SimulatorBaseTest::execWithDoubleStackSize, &simulator, &Simulator::execWithDoubleStackSize);
		connect(this, &SimulatorBaseTest::execActionStr, &simulator, &Simulator::execActionStr);
		simulatorThread.start();
	}

	void baseTest();

	void cleanup()
	{
		quitAndWait({&simulatorThread});
	}

signals:
	void execAndExpand(const common::ConfigSet & newConfig, const QSharedPointer<common::MetaData> & metaData);
	void execWithDoubleStackSize(const QSharedPointer<common::MetaData> & metaData);
	void execActionStr();


private:
	Simulator simulator;
	QThread simulatorThread;
};

void SimulatorBaseTest::baseTest()
{
	ConfigSet configSet;

	SIG_WATCHER(recResult, &simulator, &Simulator::resultReceived);
	SIG_WATCHER(recActionStr, &simulator, &Simulator::actionStrReceived);
	SIG_WATCHER(recErrorStr, &simulator, &Simulator::errorReceived);

	// * Test: wrong config, no meta data

	SIG_EXPECT(recResult,
			CHECK_AT(1, [](const ExecResult & res) {
				CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::InvalidConfig);
				CHECK_RETURN }) &
			VALUE_AT(2, nullptr)
		)

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
		CHECK_COMPARE_REGEXP(errStr, ".*(U|u)nexpected literal.*")
		CHECK_RETURN
	}))

	configSet.definitions = {Definition('A', "AX")};
	execAndExpand(configSet, nullptr);

	SIG_CHECK

	// * Test: wrong config, pass meta data

	QSharedPointer<common::MetaData> inputMeta(new common::MetaData());

	SIG_EXPECT(recResult,
		CHECK_AT(1, [](const ExecResult & res) {
			CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::InvalidConfig);
			CHECK_RETURN }) &
		CHECK_AT(2, [&inputMeta](const QSharedPointer<common::MetaData> & resMeta) {
			CHECK_COMPARE_ADDR(resMeta, inputMeta);
			CHECK_RETURN
		})
	)

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
		CHECK_COMPARE_REGEXP(errStr, ".*duplicate.*literal.*")
		CHECK_RETURN
	}))

	configSet.definitions = {Definition('A', "A"), Definition('A', "B")};
	simulator.execAndExpand(configSet, inputMeta);

	SIG_CHECK

	// * Test: just one iteration

	SIG_EXPECT(recResult,
		CHECK_AT(1, [](const ExecResult & res) {
			CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::Ok);
			CHECK_COMPARE(res.iterNum, 1);
			CHECK_RETURN })
	)

	SIG_EXPECT(recActionStr, VALUES("A[A]"))

	configSet.definitions = {Definition('A', "A[A]")};
	configSet.numIter = 1;
	simulator.execAndExpand(configSet, nullptr);
	simulator.execActionStr();

	SIG_CHECK

	// * Test: some more iterations, pass meta data

	SIG_EXPECT(recResult,
		CHECK_AT(1, [](const ExecResult & res) {
			CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::Ok);
			CHECK_COMPARE(res.iterNum, 2);
			CHECK_COMPARE(print(res.segments), "[L((0, 0), (0, -1)), L((0, -1), (1, -1)), L((1, -1), (1, 0)), L((1, 0), (0, 0))]");
			CHECK_RETURN }) &
		CHECK_AT(2, [&inputMeta](const QSharedPointer<common::MetaData> & resMeta) {
			CHECK_COMPARE_ADDR(resMeta, inputMeta);
			CHECK_RETURN
		})
	)

	SIG_EXPECT(recActionStr, VALUES("A+A+A+A"))

	configSet.definitions = {Definition('A', "A+A")};
	configSet.turn.left = 90;
	configSet.startAngle = -90;
	configSet.numIter = 2;
	configSet.stepSize = 1;
	simulator.execAndExpand(configSet, inputMeta);
	simulator.execActionStr();

	SIG_CHECK

	// * Test: Check max stack size

	SIG_EXPECT(recResult,
		CHECK_AT(1, [](const ExecResult & res) {
			CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::ExceedStackSize);
			CHECK_COMPARE(res.segments.size(), StackSize + 2);
			CHECK_COMPARE(res.iterNum, (quint32)qCeil(log(StackSize) / log(2)));
			CHECK_RETURN }) &
		VALUE_AT(2, nullptr)
	)

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
		CHECK_COMPARE_REGEXP(errStr, ".*(E|e)xceeded.*stack size.*")
		CHECK_RETURN
	}))

	configSet.definitions = {Definition('A', "AA")};
	configSet.numIter = std::numeric_limits<quint32>::max();

	simulator.execAndExpand(configSet, nullptr);

	SIG_CHECK

	// * Test: Double stack size

	SIG_EXPECT(recResult,
		CHECK_AT(1, [](const ExecResult & res) {
			CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::ExceedStackSize);
			CHECK_COMPARE(res.segments.size(), 2 * StackSize + 2);
			CHECK_COMPARE(res.iterNum, (quint32)qCeil(log(2 * StackSize) / log(2)));
			CHECK_RETURN }) &
		VALUE_AT(2, nullptr)
	)

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
		CHECK_COMPARE_REGEXP(errStr, ".*(E|e)xceeded.*stack size.*")
		CHECK_RETURN
	}))

	simulator.execWithDoubleStackSize(nullptr);

	SIG_CHECK
}

QTEST_MAIN(SimulatorBaseTest)

#include "simulator_base_test.moc"
