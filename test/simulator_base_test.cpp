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
	const int StackSize = 1000;

	Simulator simulator;
	ConfigSet configSet;
	ExecResult res;
	QSharedPointer<common::MetaData> inputMeta(new common::MetaData());
	QSharedPointer<common::MetaData> passedMeta;
	QString resActionStr;
	connect(&simulator, &Simulator::execResult, [&](const ExecResult & execResult, const QSharedPointer<common::MetaData> & metaData) {
			res = execResult;
			passedMeta = metaData; });

	connect(&simulator, &Simulator::actionStrResult, [&](const QString & actionStr) {
			resActionStr = actionStr; });

	simulator.setMaxStackSize(StackSize);

	configSet.definitions = {Definition('A', "AX")};
	simulator.execAndExpand(configSet, nullptr);
	COMPARE(res.resultKind, ExecResult::ExecResultKind::InvalidConfig);
	COMPARE((uintptr_t)passedMeta.data(), 0);

	configSet.definitions = {Definition('A', "A"), Definition('A', "B")};
	simulator.execAndExpand(configSet, inputMeta);
	COMPARE(res.resultKind, ExecResult::ExecResultKind::InvalidConfig);
	COMPARE((uintptr_t)passedMeta.data(), (uintptr_t)inputMeta.data());

	configSet.definitions = {Definition('A', "A[A]")};
	configSet.numIter = 1;
	simulator.execAndExpand(configSet, nullptr);
	COMPARE(res.resultKind, ExecResult::ExecResultKind::Ok);
	simulator.execActionStr();
	COMPARE(resActionStr, "A[A]");

	configSet.definitions = {Definition('A', "A+A")};
	configSet.turn.left = 90;
	configSet.startAngle = -90;
	configSet.numIter = 2;
	configSet.stepSize = 1;
	simulator.execAndExpand(configSet, inputMeta);
	COMPARE(res.resultKind, ExecResult::ExecResultKind::Ok);
	COMPARE(res.iterNum, 2);
	COMPARE((uintptr_t)passedMeta.data(), (uintptr_t)inputMeta.data());
	COMPARE(print(res.segments), "[L((0,0),(0,-1)),L((0,-1),(1,-1)),L((1,-1),(1,0)),L((1,0),(0,0))]");
	simulator.execActionStr();
	COMPARE(resActionStr, "A+A+A+A");

	configSet.definitions = {Definition('A', "AA")};
	configSet.numIter = std::numeric_limits<quint32>::max();
	simulator.execAndExpand(configSet, nullptr);
	COMPARE(res.resultKind, ExecResult::ExecResultKind::ExceedStackSize);
	COMPARE(res.segments.size(), StackSize + 2);
	COMPARE(res.iterNum, (quint32)qCeil(log(StackSize) / log(2)));
}

QTEST_APPLESS_MAIN(SimulatorBaseTest)

#include "simulator_base_test.moc"
