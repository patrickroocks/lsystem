#include <QtTest>

#include <simulator.h>

#include <util/print.h>
#include <util/compareutils.h>

using namespace lsystem;
using namespace lsystem::common;
using namespace util;

class SimulatorBaseTest : public QObject
{
	Q_OBJECT

private slots:
	void baseTest();

};

void SimulatorBaseTest::baseTest()
{
	Simulator simulator;
	ConfigSet configSet;

	configSet.definitions = {Definition('A', "AX")};
	COMPARE(simulator.execAndExpand(configSet), Simulator::ExecResult::InvalidConfig);

	configSet.definitions = {Definition('A', "A"), Definition('A', "B")};
	COMPARE(simulator.execAndExpand(configSet), Simulator::ExecResult::InvalidConfig);

	configSet.definitions = {Definition('A', "A[A]")};
	COMPARE(simulator.execAndExpand(configSet), Simulator::ExecResult::Ok);

	configSet.definitions = {Definition('A', "A+A")};
	configSet.turn.left = 90;
	configSet.startAngle = -90;
	configSet.numIter = 2;
	configSet.stepSize = 1;
	COMPARE(simulator.execAndExpand(configSet), Simulator::ExecResult::Ok);
	COMPARE(print(simulator.getActionStr()), "A+A+A+A");
	COMPARE(print(simulator.getSegments()), "[L((0,0),(0,-1)),L((0,-1),(1,-1)),L((1,-1),(1,0)),L((1,0),(0,0))]");

	configSet.definitions = {Definition('A', "AA")};
	configSet.numIter = std::numeric_limits<quint32>::max();
	COMPARE(simulator.execAndExpand(configSet), Simulator::ExecResult::ExceedStackSize);
	COMPARE(simulator.getSegments().size(), Simulator::DefaultMaxStackSize + 2);
	COMPARE(simulator.getLastIterNum(), (quint32)qCeil(log(Simulator::DefaultMaxStackSize) / log(2)));
}

QTEST_APPLESS_MAIN(SimulatorBaseTest)

#include "simulator_base_test.moc"
