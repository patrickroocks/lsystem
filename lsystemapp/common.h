#pragma once

#include <QColor>
#include <QPointF>
#include <QtCore>

namespace lsystem::common {

class AnimatorResultStructs : public QObject
{
	Q_OBJECT
public:
	enum class NextStepResult
	{
		Stopped,
		Restart,
		AddedOnly,
		Unchanged
	};
	Q_ENUM(NextStepResult)
};

struct AnimatorResult
{
	using NextStepResult = AnimatorResultStructs::NextStepResult;
	NextStepResult nextStepResult;
	int step = 0;
};

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
	bool move = false;
};

inline bool operator==(const Definition & lhs, const Definition & rhs)
{
	// clang-format off
	return     lhs.literal == rhs.literal
			&& lhs.command == rhs.command
			&& lhs.color   == rhs.color
			&& lhs.paint   == rhs.paint
			&& lhs.move    == rhs.move;
	// clang-format on
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
	quint8 colorNum;

	QString toString() const;

	// start/end with negated Y; when painting, the positive y-axis points
	// downward but mathematically positive y-values point upward
	QLine lineNegY() const;
	bool isPoint() const;
	QPointF pointNegY() const;
};

using LineSegs = QList<LineSeg>;

struct ConfigSet final
{
	ConfigSet() = default;
	ConfigSet(const QJsonObject & obj);
	QJsonObject toJson() const;

	Definitions definitions;

	struct TurnDegree
	{
		double left = 0;
		double right = 0;
	} turn;

	double scaling = 0;
	double startAngle = 0;
	quint32 numIter = 0;
	double stepSize = 0;
	bool valid = false;
	std::optional<int> overrideStackSize;
};

struct ConfigMap : public QMap<QString, common::ConfigSet>
{
	using QMap<QString, common::ConfigSet>::QMap;
	ConfigMap(const QJsonObject & obj);
	QJsonObject toJson() const;
};

inline bool operator==(const ConfigSet::TurnDegree & lhs, const ConfigSet::TurnDegree & rhs)
{
	return lhs.left == rhs.left && lhs.right == rhs.right;
}

inline bool operator==(const ConfigSet & lhs, const ConfigSet & rhs)
{
	// clang-format off
	return     lhs.definitions       == rhs.definitions
			&& lhs.turn              == rhs.turn
			&& lhs.scaling           == rhs.scaling
			&& lhs.startAngle        == rhs.startAngle
			&& lhs.numIter           == rhs.numIter
			&& lhs.stepSize          == rhs.stepSize
			&& lhs.valid             == rhs.valid
			&& lhs.overrideStackSize == rhs.overrideStackSize;
	// clang-format on
}

class ExecResultStructs : public QObject
{
	Q_OBJECT
public:
	enum class ExecResultKind
	{
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

	explicit ExecResult(ExecResultKind resultKind)
		: resultKind(resultKind)
	{}
	explicit ExecResult(ExecResultKind resultKind, const QVector<QColor> & actionColors)
		: resultKind(resultKind)
		, actionColors(actionColors)
	{}

	ExecResultKind resultKind = ExecResultKind::Null;
	common::LineSegs segments;
	common::LineSegs segmentsLastIter;
	quint32 iterNum = 0;
	QVector<QColor> actionColors;

	QString toString() const;
};

struct AppSettings
{
	quint32 maxStackSize = 0;

	AppSettings() = default;
	AppSettings(const QJsonObject & obj);
	QJsonObject toJson() const;
};

struct ColorGradient final
{
	ColorGradient();
	QColor colorAt(double t) const;
	QColor startColor;
	QColor endColor;
};

struct MetaData final
{
	QString toString() const;

	bool execSegments = false;
	bool execActionStr = false;

	bool showLastIter = false;
	double lastIterOpacy = 0;
	double thickness = 0;
	double opacity = 0;
	bool antiAliasing = false;
	std::optional<std::chrono::milliseconds> animLatency;
	std::optional<ColorGradient> colorGradient;
};

struct ConfigAndMeta
{
	QString toString() const;
	ConfigSet config;
	MetaData meta;
};

struct UiDrawData final
{
	QPoint offset;
	std::optional<int> drawingNumToEdit; // if not given: new drawing
	bool resultOk = false;
};

struct AllDrawData final : public lsystem::common::ConfigAndMeta
{
	UiDrawData uiDrawData;
};

inline void registerCommonTypes()
{
	qRegisterMetaType<ConfigSet>("lsystem::common::ConfigSet");
	qRegisterMetaType<ConfigSet>("common::ConfigSet");
	qRegisterMetaType<QSharedPointer<MetaData>>("QSharedPointer<lsystem::common::MetaData>");
	qRegisterMetaType<QSharedPointer<MetaData>>("QSharedPointer<common::MetaData>");
	qRegisterMetaType<QSharedPointer<ConfigAndMeta>>("QSharedPointer<lsystem::common::ConfigAndMeta>");
	qRegisterMetaType<QSharedPointer<ConfigAndMeta>>("QSharedPointer<common::ConfigAndMeta>");
	qRegisterMetaType<QSharedPointer<AllDrawData>>("QSharedPointer<lsystem::common::AllDrawData>");
	qRegisterMetaType<QSharedPointer<AllDrawData>>("QSharedPointer<common::AllDrawData>");
	qRegisterMetaType<ExecResult>("lsystem::common::ExecResult");
	qRegisterMetaType<ExecResult>("common::ExecResult");
	qRegisterMetaType<LineSegs>("lsystem::common::LineSegs");
	qRegisterMetaType<LineSegs>("common::LineSegs");
	qRegisterMetaType<AnimatorResult>("common::AnimatorResult");
	qRegisterMetaType<AnimatorResult>("lsystem::common::AnimatorResult");
}

} // namespace lsystem::common
