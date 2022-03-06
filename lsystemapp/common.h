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
	static const constexpr char * EditSettings = "show_settings";
};

struct LineSeg
{
	QPointF start;
	QPointF end;
	QColor color;

	QString toString() const;

	// start/end with negated Y; when painting, the positive y-axis points downward
	// but mathematically positive y-values point upward
	QLine lineNegY() const;
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

struct ConfigMap : public QMap<QString, common::ConfigSet>
{
	using QMap<QString, common::ConfigSet>::QMap;
	ConfigMap(const QJsonObject & obj);
	QJsonObject toJson() const;
};

inline bool operator==(const ConfigSet::TurnDegree & lhs, const ConfigSet::TurnDegree & rhs)
{
	return     lhs.left  == rhs.left
			&& lhs.right == rhs.right;
}

class ExecResultStructs : public QObject
{
	Q_OBJECT
public:
	enum class ExecResultKind {
		Null,
		Ok,
		InvalidConfig,
		ExceedStackSize
	};
	Q_ENUM(ExecResultKind)
};

struct ExecResult
{
	using ExecResultKind = ExecResultStructs::ExecResultKind;

	ExecResult() = default;
	ExecResult(ExecResultKind resultKind)
		: resultKind(resultKind)
	{}
	ExecResultKind resultKind = ExecResultKind::Null;
	common::LineSegs segments;
	common::LineSegs segmentsLastIter;
	quint32 iterNum = 0;

	QString toString() const;
};

struct AppSettings
{
	quint32 maxStackSize = 0;

	AppSettings() = default;
	AppSettings(const QJsonObject & obj);
	QJsonObject toJson() const;
};

struct MetaData
{
	virtual ~MetaData() {}
	virtual QString toString() const;

	bool showLastIter = false;
	double lastIterOpacy = 0;
	double thickness = 0;
	double opacity = 0;
	bool antiAliasing = false;
};

inline void registerCommonTypes()
{
	qRegisterMetaType<ConfigSet>("lsystem::common::ConfigSet");
	qRegisterMetaType<ConfigSet>("common::ConfigSet");
	qRegisterMetaType<QSharedPointer<MetaData>>("QSharedPointer<lsystem::common::MetaData>");
	qRegisterMetaType<QSharedPointer<MetaData>>("QSharedPointer<common::MetaData>");
	qRegisterMetaType<ExecResult>("lsystem::common::ExecResult");
	qRegisterMetaType<ExecResult>("common::ExecResult");
	qRegisterMetaType<LineSegs>("lsystem::common::LineSegs");
	qRegisterMetaType<LineSegs>("common::LineSegs");
}

}

#endif // COMMON_H
