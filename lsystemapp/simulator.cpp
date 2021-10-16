#include "simulator.h"

#include <util/qt-cont-utils.h>
#include <util/print.h>

#include <math.h>

using namespace util;

namespace lsystem {

using namespace common;
using namespace impl;

void ProcessLiteralAction::expand() const
{
	for (const DynAction & act : qAsConst(subActions)) {
		actInt.addAction(act.data());
	}
}

void ProcessLiteralAction::exec(State & state) const
{
	if (!paint) return;

	LineSeg seg;
	seg.start = state.cur;
	seg.end   = state.cur += state.d;
	seg.color = color;
	actInt.addSegment(seg);
}

void ScaleStartAction::exec(State & state) const
{
	state.subStates.push((StateGeom &)state);
	state.d *= scaleFct;
}

void ScaleStopAction::exec(State & state) const
{
	(StateGeom &)state = state.subStates.pop();
}

void TurnAction::exec(State & state) const
{
	const double newDx =
				 tCos * state.d.x() - tSin * state.d.y();
	state.d.setY(tSin * state.d.x() + tCos * state.d.y());
	state.d.setX(newDx);
}

double TurnAction::roundNearWhole(double val)
{
	// round values near to "0.5 * x"
	const double tmp = 2 * val;
	const double tmpRound = qRound(tmp);
	if (qAbs(tmp - tmpRound) < 1E-12) {
		return tmpRound / 2;
	} else {
		return val;
	}
}

// -------------------------------------------------------------------------------------

void Simulator::execAndExpand(const common::ConfigSet & newConfig, const QSharedPointer<MetaData> & metaData)
{
	curMaxStackSize = maxStackSize;

	if (!(validConfig && expansionEqual(newConfig))) {
		validConfig = parseActions(newConfig);
		if (!validConfig) {
			emit execResult(ExecResult::ExecResultKind::InvalidConfig, metaData);
			return;
		}
	}

	config = newConfig;
	execIterations(metaData);
}

void Simulator::execWithDoubleStackSize(const QSharedPointer<MetaData> & metaData)
{
	curMaxStackSize *= 2;
	execIterations(metaData);
}

void Simulator::execActionStr()
{
	QString actionStr;
	for (const Action * action : qAsConst(currentActions)) actionStr += print(action);
	emit actionStrResult(actionStr);
}

void Simulator::execIterations(const QSharedPointer<MetaData> & metaData)
{
	ExecResult res;

	currentActions = {startAction.data()};
	nextActions.clear();

	for (quint32 i = 0 ; i < config.numIter ; ++i) {
		if (!execIter()) {
			res.resultKind = ExecResult::ExecResultKind::ExceedStackSize;
			res.segments = getSegments();
			res.iterNum = i + 1;
			emit showError(QString("Exceeded maximum stack size (%1) at iteration %2, <a href=\"%3\">Paint with stack size %4</a>")
					.arg(curMaxStackSize).arg(res.iterNum).arg(Links::NextIterations).arg(2 * curMaxStackSize));
			emit execResult(res, metaData);
			return;
		}
	}

	res.resultKind = ExecResult::ExecResultKind::Ok;
	res.iterNum = config.numIter;
	res.segments = getSegments();
	emit execResult(res, metaData);
}

bool Simulator::execIter()
{
	auto takeNextActions = [&]() {
		currentActions.clear();
		qSwap(currentActions, nextActions);
	};

	for (const Action * actPtr : qAsConst(currentActions)) {
		if (nextActions.size() > curMaxStackSize) {
			takeNextActions();
			return false;
		}
		actPtr->expand();
	}

	takeNextActions();
	return true;
}

LineSegs Simulator::getSegments()
{
	segments.clear();

	const double startTurn = qDegreesToRadians(config.startAngle);
	TurnAction initialTurnAct(*this, std::cos(startTurn), std::sin(startTurn), '\0');
	State state;
	state.d.setX(config.stepSize);
	initialTurnAct.exec(state);

	for (const Action * act : qAsConst(currentActions)) {
		act->exec(state);
	}

	return segments;
}

void Simulator::setMaxStackSize(int newMaxStackSize)
{
	maxStackSize = newMaxStackSize;
}

void Simulator::addAction(const Action * action)
{
	nextActions << action;
}

void Simulator::addSegment(const LineSeg & seg)
{
	segments << seg;
}

bool Simulator::parseActions(const ConfigSet & newConfig)
{
	mainActions.clear();
	startAction = nullptr;

	QMap<char, DynAction> allActions;
	auto addAction = [&allActions](const DynAction & action) {
		allActions[action->getLiteral()] = action;
	};

	// * turns
	const double radTurnLeft  = qDegreesToRadians(newConfig.turn.left);
	const double radTurnRight = qDegreesToRadians(newConfig.turn.right);
	DynTurnAction turnLeft(new  TurnAction(*this, std::cos(radTurnLeft),  std::sin(radTurnLeft),  '+'));
	DynTurnAction turnRight(new TurnAction(*this, std::cos(radTurnRight), std::sin(radTurnRight), '-' ));
	addAction(turnLeft);
	addAction(turnRight);

	// * scale start/end
	DynAction scaleStart(new ScaleStartAction(*this, newConfig.scaling, '['));
	DynAction scaleEnd(new ScaleStopAction(*this, ']'));
	addAction(scaleStart);
	addAction(scaleEnd);

	// * main actions
	for (const Definition & def : newConfig.definitions) {
		if (mainActions.contains(def.literal)) {
			emit showError(printStr("Found duplicate definition for literal '%1'", def.literal));
			return false;
		}
		DynProcessLiteralAction & mainAction = mainActions[def.literal];
		mainAction = DynProcessLiteralAction::create(*this, def.literal, def.color, def.paint);
		addAction(mainAction);

		// first action is start action
		if (!startAction) startAction = mainAction;
	}

	for (const Definition & def : newConfig.definitions) {
		DynProcessLiteralAction literalAction = mainActions[def.literal];

		qint16 scaleLevel = 0;
		for (const QChar & qc : def.command) {
			char c = qc.toLatin1();

			if (!allActions.contains(c)) {
				emit showError(printStr("Unexpected literal '%1' in actions for literal '%2'", qc, def.literal));
				return false;
			}

			literalAction->subActions << allActions[c];
			if      (c == '[') ++scaleLevel;
			else if (c == ']') --scaleLevel;
		}

		if (scaleLevel != 0) {
			emit showError(printStr("Scale down/up, i.e., '[' and ']' symbols do not match in actions for literal '%1': %2",
					def.literal, def.command));
			return false;
		}
	}

	return true;
}

bool Simulator::expansionEqual(const ConfigSet & newConfig) const
{
	return     newConfig.definitions == config.definitions
			&& newConfig.numIter     == config.numIter
			&& newConfig.turn        == config.turn
			&& newConfig.scaling     == config.scaling;
}

void Action::expand() const
{
	actInt.addAction(this);
}

}
