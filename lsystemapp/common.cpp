#include "common.h"

#include <util/qt-cont-utils.h>
#include <util/print.h>

using namespace util;

namespace lsystem::common {

Definition::Definition(const QJsonObject & obj)
{
	const QByteArray literalStr = obj["literal"].toString().toLatin1();
	literal = literalStr.size() == 1 ? literalStr.data()[0] : '\0';
	command = obj["command"].toString();
	color   = obj["color"].toInt();
	paint   = obj["paint"].toBool();
}

Definition::Definition(char literal, const QString & command)
	: literal(literal),
	  command(command),
	  paint(true)
{
}

QJsonObject Definition::toJson() const
{
	QJsonObject rv;
	rv["literal"] = QString(literal);
	rv["command"] = command;
	rv["color"]   = color.value();
	rv["paint"]   = paint;
	return rv;
}

// ---------------------------------------------------------------------------

QString LineSeg::toString() const
{
	return printStr("L(%1,%2)", start, end);
}

// ---------------------------------------------------------------------------

ConfigSet::ConfigSet(const QJsonObject & obj)
	: turn({     obj["turnLeft"]  .toDouble(),
				 obj["turnRight"] .toDouble()}),
	  scaling(   obj["scaling"]   .toDouble()),
	  startAngle(obj["startAngle"].toDouble()),
	  numIter(   obj["numIter"]   .toInt()),
	  stepSize(  obj["stepSize"]  .toDouble())
{
	for (const auto & jsonDef : obj["definitions"].toArray()) {
		definitions << Definition(jsonDef.toObject());
	}
}

QJsonObject ConfigSet::toJson() const
{
	QJsonObject rv;

	rv["turnLeft"]   = turn.left;
	rv["turnRight"]  = turn.right;
	rv["scaling"]    = scaling;
	rv["startAngle"] = startAngle;
	rv["stepSize"]   = stepSize;
	rv["numIter"]    = (int)numIter;

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
		(*this)[key] = ConfigSet(value.toJsonObject());
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

AppSettings::AppSettings(const QJsonObject & obj)
	: maxStackSize(obj["maxStackSize"].toInt())
{
}

QJsonObject AppSettings::toJson() const
{
	QJsonObject rv;
	rv["maxStackSize"] = (int)maxStackSize;
	return rv;
}

}
