#include "simulator.h"

#include <util/print.h>
#include <util/qtcontutils.h>

using namespace util;

namespace lsystem {

using namespace common;
using namespace impl;

void ProcessLiteralAction::expand() const
{
	for (const DynAction & act : std::as_const(subActions)) {
		simInt.addAction(act.data());
	}
}

void ProcessLiteralAction::exec(State & state) const
{
	auto lastState = state.cur;

	if (move) {
		state.cur += state.d;
	}

	if (paint) {
		simInt.addSegment(LineSeg{.start = lastState, .end = state.cur, .colorNum = colorNum});
	}
}

void ScaleStartAction::exec(State & state) const
{
	state.subStates.push((StateGeom &) state);
	state.d *= scaleFct;
}

void ScaleStopAction::exec(State & state) const { (StateGeom &) state = state.subStates.pop(); }

void TurnAction::exec(State & state) const
{
	const double newDx = tCos * state.d.x() - tSin * state.d.y();
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

void Simulator::exec(const QSharedPointer<AllDrawData> & data)
{
	const auto & meta = data->meta;

	if (!meta.execActionStr && !meta.execSegments) {
		emit errorReceived("Nothing to execute, neither segments nor action string is requested to calculate.");
		return;
	}

	// Executes the config with the given meta data.
	// Instead of a naive recalculation, we try to use as much as we can from the
	// previous results

	const common::ConfigSet & newConfig = data->config;

	// set stack size for current execution, might be overridden
	curMaxStackSize = newConfig.overrideStackSize ? *newConfig.overrideStackSize : maxStackSize;

	const bool expandedActionsEqual = expansionEqual(newConfig);

	// The actual expansion is equal if:
	// * the expanded actions are equal,
	// * and the execution was not stopped due to StackSize.
	const bool executedSameExpansion = expandedActionsEqual && !stackSizeLimitReached;

	if (!executedSameExpansion) actionStr = "";

	// Check for valid config and parse the actions.
	if (!(validConfig && expandedActionsEqual)) {
		// parseAction raises errorReceived
		validConfig = parseActions(newConfig);
		if (!validConfig) {
			if (meta.execSegments) emit segmentsReceived(ExecResult{ExecResult::ExecResultKind::InvalidConfig}, data);
			if (meta.execActionStr) emit actionStrReceived("(error occurred)");
			return;
		}
	}

	// We don't need the full execIterations if:
	// * we don't show the last iteration (for this we need the loop in `execIterations`),
	//  - only relevant if segments are shown, there is no "show last iteration for action strings"
	// * and the actual expansion is equal (see above).

	ExecResult res{ExecResult::ExecResultKind::Ok, actionColors};
	res.iterNum = config.numIter;

	if (executedSameExpansion && !(meta.showLastIter && meta.execSegments)) {
		if (config == newConfig) {
			// If the configs are completely identical, we just use the last result:
			res.segments = segments;
		} else {
			// We take the new config, but don't have to do the expansion again.
			// Recalulating the segments is enough, if, e.g., the step size or start angle changes.
			config = newConfig;
			res.segments = getSegments();
		}
		// Action String cannot change in this case, only recalculate if not present.
		if (meta.execActionStr && actionStr.isEmpty()) composeActionStr();
	} else {
		// We have to reprocess everything.
		// Iterations are needed for segments and action string.
		config = newConfig;
		execIterations(meta, res);

		if (meta.execActionStr) composeActionStr();
	}

	// Finally report the results.
	if (meta.execSegments) emit segmentsReceived(res, data);
	if (meta.execActionStr) emit actionStrReceived(actionStr);
}

void Simulator::composeActionStr()
{
	actionStr = "";
	for (const Action * action : std::as_const(currentActions)) actionStr += print(action);
}

void Simulator::execIterations(const common::MetaData & meta, ExecResult & res)
{
	currentActions = {startAction.data()};
	nextActions.clear();

	for (quint32 curIter = 1; curIter <= config.numIter; ++curIter) {
		if (!execOneIteration()) {
			res.resultKind = ExecResult::ExecResultKind::ExceedStackSize;
			res.segments = getSegments();
			res.iterNum = curIter;
			stackSizeLimitReached = true;
			emit errorReceived(QString("Exceeded maximum stack size (%1) at iteration %2, <a "
									   "href=\"%3\">Paint with stack size %4</a>, <a "
									   "href=\"%5\">Edit settings</a>")
								   .arg(curMaxStackSize)
								   .arg(res.iterNum)
								   .arg(Links::NextIterations)
								   .arg(2 * curMaxStackSize)
								   .arg(Links::EditSettings));
			return;
		} else if (meta.showLastIter && curIter == config.numIter - 1) {
			res.segmentsLastIter = getSegments();
		}
	}

	stackSizeLimitReached = false;
	res.iterNum = config.numIter;
	res.segments = getSegments();
}

bool Simulator::execOneIteration()
{
	const auto takeNextActions = [&]() {
		currentActions.clear();
		qSwap(currentActions, nextActions);
	};

	for (const Action * actPtr : std::as_const(currentActions)) {
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

	for (const Action * act : std::as_const(currentActions)) {
		act->exec(state);
	}

	return segments;
}

void Simulator::setMaxStackSize(int newMaxStackSize) { maxStackSize = newMaxStackSize; }

void Simulator::addAction(const Action * action) { nextActions << action; }

void Simulator::addSegment(const LineSeg & seg) { segments << seg; }

bool Simulator::parseActions(const ConfigSet & newConfig)
{
	actionColors.clear();
	mainActions.clear();
	startAction = nullptr;

	if (!newConfig.valid) {
		// should not happen, ConfigSets with "valid == false" are semantically
		// NULL-configs.
		emit errorReceived("Received NULL-config (valid == false)");
		return false;
	}

	if (newConfig.definitions.isEmpty()) {
		emit errorReceived("No literals given");
		return false;
	}

	QMap<char, DynAction> allActions;
	auto addAction = [&allActions](const DynAction & action) { allActions[action->getLiteral()] = action; };

	// * turns
	const double radTurnLeft = qDegreesToRadians(newConfig.turn.left);
	const double radTurnRight = qDegreesToRadians(newConfig.turn.right);
	DynTurnAction turnLeft(new TurnAction(*this, std::cos(radTurnLeft), std::sin(radTurnLeft), '+'));
	DynTurnAction turnRight(new TurnAction(*this, std::cos(radTurnRight), std::sin(radTurnRight), '-'));
	addAction(turnLeft);
	addAction(turnRight);

	// * scale start/end
	DynAction scaleStart(new ScaleStartAction(*this, newConfig.scaling, '['));
	DynAction scaleEnd(new ScaleStopAction(*this, ']'));
	addAction(scaleStart);
	addAction(scaleEnd);

	// * main actions

	// create dynamic actions, create color map
	for (const Definition & def : newConfig.definitions) {
		if (mainActions.contains(def.literal)) {
			emit errorReceived(printStr("Found duplicate definition for literal '%1'", def.literal));
			return false;
		}
		DynProcessLiteralAction & mainAction = mainActions[def.literal];

		auto itCol = std::find(actionColors.begin(), actionColors.end(), def.color);
		if (itCol == actionColors.end()) {
			actionColors.push_back(def.color);
			itCol = actionColors.end() - 1;
		}

		mainAction = DynProcessLiteralAction::create(*this, def.literal, itCol - actionColors.begin(), def.paint, def.move);
		addAction(mainAction);

		// first action is start action
		if (!startAction) startAction = mainAction;
	}

	// compose commands for dynamic action (including links to subactions)
	for (const Definition & def : newConfig.definitions) {
		DynProcessLiteralAction literalAction = mainActions[def.literal];

		qint16 scaleLevel = 0;
		for (const QChar & qc : def.command) {
			char c = qc.toLatin1();

			if (!allActions.contains(c)) {
				emit errorReceived(printStr("Unexpected literal '%1' in actions for literal '%2'", qc, def.literal));
				return false;
			}

			literalAction->subActions << allActions[c];
			if (c == '[') ++scaleLevel;
			else if (c == ']') --scaleLevel;
		}

		if (scaleLevel != 0) {
			emit errorReceived(printStr("Scale down/up, i.e., '[' and ']' symbols do "
										"not match in actions for literal '%1': %2",
										def.literal,
										def.command));
			return false;
		}
	}

	return true;
}

bool Simulator::expansionEqual(const ConfigSet & newConfig) const
{
	return newConfig.definitions == config.definitions && newConfig.numIter == config.numIter && newConfig.turn == config.turn
		   && newConfig.scaling == config.scaling;
}

void Action::expand() const { simInt.addAction(this); }

} // namespace lsystem
