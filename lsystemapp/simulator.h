#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <common.h>

namespace lsystem {

namespace impl {

class Action;
using DynAction = QSharedPointer<Action>;
using DynActionList = QList<DynAction>;
class ProcessLiteralAction;
using DynProcessLiteralAction = QSharedPointer<ProcessLiteralAction>;

struct StateGeom
{
	QPointF cur;
	QPointF d;
};

struct State : public StateGeom
{
	QStack<StateGeom> subStates;
};

using States = QList<State>;

// ----------------------------------------------------------------------

class ActionInterface
{
public:
	virtual ~ActionInterface() {}
	virtual void addAction(const Action * state) = 0;
	virtual void addSegment(const common::LineSeg & seg) = 0;
};

class Action
{
public:
	Action(ActionInterface & actInt, char literal)
		: actInt(actInt),
		  literal(literal)
	{}
	virtual ~Action() {}

	virtual void expand() const;
	virtual void exec(State & state) const = 0;
	char getLiteral() const { return literal; }
	QString toString() const { return QString(1, literal); }

protected:
	ActionInterface & actInt;

private:
	char literal;
};

class ProcessLiteralAction : public Action
{
public:
	ProcessLiteralAction(ActionInterface & actInt, char literal, QColor color, bool paint)
		: Action(actInt, literal),
		  color(color),
		  paint(paint)
	{}

	void expand() const override;
	void exec(State & state) const override;

public:
	DynActionList subActions;

private:
	QColor color;
	bool paint = false;
};

class TurnAction : public Action
{
public:
	TurnAction(ActionInterface & actInt, double tCos, double tSin, char literal)
		: Action(actInt, literal),
		  tCos(roundNearWhole(tCos)),
		  tSin(roundNearWhole(tSin))
	{}

	void exec(State & state) const override;

private:
	static double roundNearWhole(double val);

private:
	const double tCos;
	const double tSin;
};

using DynTurnAction = QSharedPointer<TurnAction>;

class ScaleStartAction : public Action
{
public:
	ScaleStartAction(ActionInterface & actInt, double scaleFct, char literal)
		: Action(actInt, literal),
		  scaleFct(scaleFct)
	{}

	void exec(State & state) const override;

private:
	const double scaleFct;
};

class ScaleStopAction : public Action
{
public:
	ScaleStopAction(ActionInterface & actInt, char literal)
		: Action(actInt, literal)
	{}

	void exec(State & state) const override;
};

}

// ---------------------------------------------------------------------------------------------------------

class Simulator : public QObject, public impl::ActionInterface
{
	Q_OBJECT

public:
	enum class ExecResult {
		Ok,
		InvalidConfig,
		ExceedStackSize
	};
	Q_ENUM(ExecResult)

	ExecResult execAndExpand(const common::ConfigSet & newConfig);
	ExecResult execWithDoubleStackSize();

	bool isValid();
	QString getLastError() const { return lastError; }
	common::LineSegs getSegments();
	QString getActionStr() const;
	quint32 getLastIterNum() const { return lastIterNum; }

public:
	static const constexpr int DefaultMaxStackSize = 100000;

private:
	void addAction(const impl::Action * action) override;
	void addSegment(const common::LineSeg & seg) override;

	bool parseActions(const common::ConfigSet & newConfig);
	bool expansionEqual(const common::ConfigSet & newConfig) const;
	ExecResult execIterations();
	bool execIter();

private:
	bool valid = false;
	common::ConfigSet config;
	common::LineSegs segments;

	QList<const impl::Action *> currentActions;
	QList<const impl::Action *> nextActions;

	int curMaxStackSize = DefaultMaxStackSize;
	QString lastError;
	quint32 lastIterNum = 0;

	QMap<char, impl::DynProcessLiteralAction> mainActions;
	impl::DynProcessLiteralAction startAction;
};

}

#endif // SIMULATOR_H
