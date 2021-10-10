#ifndef COMMON_H
#define COMMON_H

#include <QtCore>
#include <QColor>
#include <QPointF>

namespace lsystem::common {

struct Definition
{
	Definition() = default;
	explicit Definition(char literal, const QString & command);
	Definition(const QJsonObject & obj);
	QJsonObject toJson() const;

	char literal = '\0';
	QString command;
	QColor color;
	bool paint = false;
};

inline bool operator==(const Definition & lhs, const Definition & rhs)
{
	return     lhs.literal == rhs.literal
			&& lhs.command == rhs.command
			&& lhs.color   == rhs.color
			&& lhs.paint   == rhs.paint;
}

using Definitions = QList<Definition>;

struct Links
{
	static const constexpr char * NextIterations = "next_iter";
	static const constexpr char * ShowSymbols = "show_symbols";
};

struct LineSeg
{
	QPointF start;
	QPointF end;
	QColor color;

	QString toString() const;
};

using LineSegs = QList<LineSeg>;

struct ConfigSet
{
	ConfigSet() = default;
	ConfigSet(const QJsonObject & obj);
	QJsonObject toJson() const;

	Definitions definitions;
	struct TurnDegree {
		double left = 0;
		double right = 0;
	} turn;
	double scaling = 0;
	double startAngle = 0;
	quint32 numIter = 0;
	double stepSize = 0;
	bool valid = false;
};

inline bool operator==(const ConfigSet::TurnDegree & lhs, const ConfigSet::TurnDegree & rhs)
{
	return     lhs.left  == rhs.left
			&& lhs.right == rhs.right;
}

}

#endif // COMMON_H
