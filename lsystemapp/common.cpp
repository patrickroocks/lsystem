#include "common.h"

#include <jsonkeys.h>
#include <util/print.h>
#include <util/qtcontutils.h>

#include <QtMath>

using namespace util;
using namespace lsystem::constants;

namespace lsystem::common {

Definition::Definition(const QJsonObject & obj)
{
	const QByteArray literalStr = obj[JsonKeyLiteral].toString().toLatin1();
	literal = literalStr.size() == 1 ? literalStr.data()[0] : '\0';
	command = obj[JsonKeyCommand].toString();
	color = obj[JsonKeyColor].toInt();
	paint = obj[JsonKeyPaint].toBool();
	move = obj[JsonKeyMove].toBool();
}

Definition::Definition(char literal, const QString & command)
	: literal(literal)
	, command(command)
	, paint(true)
	, move(true)
{}

QJsonObject Definition::toJson() const
{
	QJsonObject rv;
	rv[JsonKeyLiteral] = QString(literal);
	rv[JsonKeyCommand] = command;
	rv[JsonKeyColor] = color.value();
	rv[JsonKeyPaint] = paint;
	rv[JsonKeyMove] = move;
	return rv;
}

// ---------------------------------------------------------------------------

QString LineSeg::toString() const { return printStr("L(%1, %2)", start, end); }

QLine LineSeg::lineNegY() const { return QLine(start.x(), -start.y(), end.x(), -end.y()); }

bool LineSeg::isPoint() const { return start == end; }

QPointF LineSeg::pointNegY() const { return QPoint(start.x(), -start.y()); }

// ---------------------------------------------------------------------------

ConfigSet::ConfigSet(const QJsonObject & obj)
	: turn({obj[JsonKeyTurnLeft].toDouble(), obj[JsonKeyTurnRight].toDouble()})
	, scaling(obj[JsonKeyScaling].toDouble())
	, startAngle(obj[JsonKeyStartAngle].toDouble())
	, numIter(obj[JsonKeyNumIter].toInt())
	, stepSize(obj[JsonKeyStepSize].toDouble())
{
	for (const auto & jsonDef : obj["definitions"].toArray()) {
		definitions << Definition(jsonDef.toObject());
	}
}

QJsonObject ConfigSet::toJson() const
{
	QJsonObject rv;

	rv[JsonKeyTurnLeft] = turn.left;
	rv[JsonKeyTurnRight] = turn.right;
	rv[JsonKeyScaling] = scaling;
	rv[JsonKeyStartAngle] = startAngle;
	rv[JsonKeyStepSize] = stepSize;
	rv[JsonKeyNumIter] = static_cast<int>(numIter);

	QJsonArray jsonDefinitions;
	for (const Definition & def : definitions) {
		jsonDefinitions << def.toJson();
	}
	rv["definitions"] = jsonDefinitions;

	return rv;
}

// ----------------------------------------------------------------------------

ConfigMap::ConfigMap(const QJsonObject & obj)
{
	QVariantMap tmpMap = obj.toVariantMap();
	for (const auto & [key, value] : KeyVal(tmpMap)) {
		auto & entry = (*this)[key];
		entry = ConfigSet(value.toJsonObject());
		entry.name = key;
	}
}

QJsonObject ConfigMap::toJson() const
{
	QJsonObject rv;
	for (const auto & [key, value] : KeyVal(*this)) {
		rv[key] = value.toJson();
	}
	return rv;
}

// ----------------------------------------------------------------------------

QString ExecResult::toString() const { return printStr("ExecResult(%1, segments: %2, iterations: %3)", resultKind, segments, iterNum); }

// ----------------------------------------------------------------------------

AppSettings::AppSettings(const QJsonObject & obj)
	: maxStackSize(obj[JsonKeySettingsMaxStackSize].toInt())
{}

QJsonObject AppSettings::toJson() const
{
	QJsonObject rv;
	rv[JsonKeySettingsMaxStackSize] = static_cast<int>(maxStackSize);
	return rv;
}

// ----------------------------------------------------------------------------

ColorGradient::ColorGradient() { setDefault(); }

void ColorGradient::setDefault()
{
	startColor = QColor(255, 0, 0);
	endColor = QColor(0, 0, 255);
}

QColor ColorGradient::colorAt(double t) const
{
	const double u = 1.0 - t;
	return QColor(static_cast<int>(qFloor(startColor.red() * u + endColor.red() * t)),
				  static_cast<int>(qFloor(startColor.green() * u + endColor.green() * t)),
				  static_cast<int>(qFloor(startColor.blue() * u + endColor.blue() * t)));
}

// ----------------------------------------------------------------------------

QString MetaData::toString() const
{
	return printStr("MetaData(execSegments: %1, execActionStr: %2, showLastIter: %3, lastIterOpacy: %4, thickness: %5, opacity: %6, "
					"antiAliasing: %7, animLatency: %8)",
					execSegments,
					execActionStr,
					showLastIter,
					lastIterOpacy,
					thickness,
					opacity,
					antiAliasing,
					animLatency);
}

// ----------------------------------------------------------------------------

QString ConfigAndMeta::toString() const { return printStr("[%1, %2]", QJsonDocument{config.toJson()}.toJson(), meta.toString()); }

} // namespace lsystem::common
