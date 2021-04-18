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

Simulator::ExecResult Simulator::execAndExpand(const common::ConfigSet & newConfig)
{
	curMaxStackSize = DefaultMaxStackSize;

	if (!(valid && expansionEqual(newConfig))) {

		valid = parseActions(newConfig);
		if (!valid) return ExecResult::Invalid;
	}

	config = newConfig;

	return execIterations();
}

Simulator::ExecResult Simulator::execWithDoubleStackSize()
{
	curMaxStackSize *= 2;
	return execIterations();
}

bool Simulator::isValid()
{
	return valid;
}

Simulator::ExecResult Simulator::execIterations()
{
	currentActions = {mainActions.first().data()};
	nextActions.clear();

	for (quint32 i = 0 ; i < config.numIter ; ++i) {
		if (!execIter()) {
			lastIterNum = i + 1;
			lastError = QString("Exceeded maximum stack size (%1) at iteration %2, <a href=\"%3\">Paint with stack size %4</a>")
				.arg(curMaxStackSize).arg(lastIterNum).arg(Links::NextIterations).arg(2 * curMaxStackSize);
			return ExecResult::ExceedStackSize;
		}
	}

	lastIterNum = config.numIter;
	return ExecResult::Ok;
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

QString Simulator::getActionStr() const
{
	QString rv;
	for (const Action * action : currentActions) rv += print(action);
	return rv;
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
	for (const auto & [key, value] : KeyVal(newConfig.definitions)) {
		DynProcessLiteralAction & mainAction = mainActions[key];
		mainAction = DynProcessLiteralAction::create(*this, key, value.color, value.paint);
		addAction(mainAction);
	}

	for (const auto & [key, value] : KeyVal(newConfig.definitions)) {
		DynProcessLiteralAction literalAction = mainActions[key];

		qint16 scaleLevel = 0;
		for (const QChar & qc : value.command) {
			char c = qc.toLatin1();

			if (!allActions.contains(c)) {
				lastError = QString("unexpected literal '%1' in actions for literal '%2'").arg(qc).arg(key);
				return false;
			}

			literalAction->subActions << allActions[c];
			if      (c == '[') ++scaleLevel;
			else if (c == ']') --scaleLevel;
		}

		if (scaleLevel != 0) {
			lastError = QString("scale down/up, i.e., '[' and ']' symbols do not match in actions for literal '%1': %2").arg(key).arg(value.command);
			return false;
		}
	}

	return true;
}

bool Simulator::expansionEqual(const ConfigSet & newConfig)
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
