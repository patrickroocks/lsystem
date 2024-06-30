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
		connect(this, &SimulatorBaseTest::exec, &simulator, &Simulator::exec);
		simulatorThread.start();
	}

	void baseTest();

	void cleanup()
	{
		quitAndWait({&simulatorThread});
	}

signals:
	void exec(const QSharedPointer<common::AllDrawData> & data);

private:
	Simulator simulator;
	QThread simulatorThread;
};

void SimulatorBaseTest::baseTest()
{
	SIG_WATCHER(recResult, &simulator, &Simulator::segmentsReceived);
	SIG_WATCHER(recActionStr, &simulator, &Simulator::actionStrReceived);
	SIG_WATCHER(recErrorStr, &simulator, &Simulator::errorReceived);

	// Base data
	QSharedPointer<common::AllDrawData> inputData = QSharedPointer<common::AllDrawData>::create();
	inputData->config.valid = true;

	// * Test missing execution task

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
				   CHECK_COMPARE_REGEXP(errStr, ".*(N|n)othing to execute.*")
				   CHECK_RETURN
			   }))

	emit exec(inputData);

	SIG_CHECK

	// * Set execution task segments from here
	inputData->meta.execSegments = true;

	// * Test: wrong config

	inputData->config.definitions = {Definition('A', "AX")};

	SIG_EXPECT(recResult, CHECK_AT(1, [](const ExecResult & res) {
							  CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::InvalidConfig);
							  CHECK_RETURN
						  }) & CHECK_AT(2, [&inputData](const QSharedPointer<common::AllDrawData> & resMeta) {
							  CHECK_VERIFY(resMeta);
							  CHECK_COMPARE_ADDR(resMeta, inputData);
							  CHECK_RETURN
						  }))

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
				   CHECK_COMPARE_REGEXP(errStr, ".*(U|u)nexpected literal.*")
				   CHECK_RETURN
			   }))

	emit exec(inputData);

	SIG_CHECK

	// * Test: wrong config

	SIG_EXPECT(recResult, CHECK_AT(1, [](const ExecResult & res) {
							  CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::InvalidConfig);
							  CHECK_RETURN
						  }) & CHECK_AT(2, [&inputData](const QSharedPointer<common::AllDrawData> & resMeta) {
							  CHECK_COMPARE_ADDR(resMeta, inputData);
							  CHECK_RETURN
						  }))

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
				   CHECK_COMPARE_REGEXP(errStr, ".*duplicate.*literal.*")
				   CHECK_RETURN
			   }))

	inputData->config.definitions = {Definition('A', "A"), Definition('A', "B")};
	emit exec(inputData);

	SIG_CHECK

	// * From here we test with action strings.
	inputData->meta.execActionStr = true;

	// * Test: just one iteration

	SIG_EXPECT(recResult, CHECK_AT(1, [](const ExecResult & res) {
				   CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::Ok);
				   CHECK_COMPARE(res.iterNum, 1);
				   CHECK_RETURN
			   }))

	SIG_EXPECT(recActionStr, VALUES("A[A]"))

	auto & configSet = inputData->config;
	configSet.definitions = {Definition('A', "A[A]")};
	configSet.numIter = 1;
	emit exec(inputData);

	SIG_CHECK

	// * Test: some more iterations, pass meta data

	SIG_EXPECT(recResult, CHECK_AT(1, [](const ExecResult & res) {
							  CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::Ok);
							  CHECK_COMPARE(res.iterNum, 2);
							  CHECK_COMPARE(print(res.segments),
											"(L((0, 0), (0, -1)), L((0, -1), (1, -1)), L((1, -1), (1, 0)), L((1, 0), (0, 0)))");
							  CHECK_RETURN
						  }) & CHECK_AT(2, [&inputData](const QSharedPointer<common::AllDrawData> & resMeta) {
							  CHECK_COMPARE_ADDR(resMeta, inputData);
							  CHECK_RETURN
						  }))

	SIG_EXPECT(recActionStr, VALUES("A+A+A+A"))

	configSet.definitions = {Definition('A', "A+A")};
	configSet.turn.left = 90;
	configSet.startAngle = -90;
	configSet.numIter = 2;
	configSet.stepSize = 1;
	emit exec(inputData);

	SIG_CHECK

	// * From here we stop testing with action strings.
	inputData->meta.execActionStr = false;

	// * Test: Check max stack size

	SIG_EXPECT(recResult, CHECK_AT(1, [](const ExecResult & res) {
				   CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::ExceedStackSize);
				   CHECK_COMPARE(res.segments.size(), StackSize + 2);
				   CHECK_COMPARE(res.iterNum, (quint32) qCeil(log(StackSize) / log(2)));
				   CHECK_RETURN
			   }))

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
				   CHECK_COMPARE_REGEXP(errStr, ".*(E|e)xceeded.*stack size.*")
				   CHECK_RETURN
			   }))

	configSet.definitions = {Definition('A', "AA")};
	configSet.numIter = std::numeric_limits<quint32>::max();

	emit exec(inputData);

	SIG_CHECK

	// * Test: Double stack size

	configSet.overrideStackSize = 2 * StackSize;

	SIG_EXPECT(recResult, CHECK_AT(1, [](const ExecResult & res) {
				   CHECK_COMPARE(res.resultKind, ExecResult::ExecResultKind::ExceedStackSize);
				   CHECK_COMPARE(res.segments.size(), 2 * StackSize + 2);
				   CHECK_COMPARE(res.iterNum, (quint32) qCeil(log(2 * StackSize) / log(2)));
				   CHECK_RETURN
			   }))

	SIG_EXPECT(recErrorStr, CHECK([](const QString & errStr) {
				   CHECK_COMPARE_REGEXP(errStr, ".*(E|e)xceeded.*stack size.*")
				   CHECK_RETURN
			   }))

	emit exec(inputData);

	SIG_CHECK
}

QTEST_MAIN(SimulatorBaseTest)

#include "simulator_base_test.moc"
