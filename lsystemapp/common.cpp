#include "common.h"

#include <util/qt-cont-utils.h>
#include <util/print.h>

using namespace util;

namespace lsystem::common {

QString LineSeg::toString() const
{
	return printStr("L(%1,%2)", start, end);
}

ConfigSet::ConfigSet(const QJsonObject & obj)
	: turn({     obj["turnLeft"]  .toDouble(),
				 obj["turnRight"] .toDouble()}),
	  scaling(   obj["scaling"]   .toDouble()),
	  startAngle(obj["startAngle"].toDouble()),
	  numIter(obj["numIter"].toInt()),
	  stepSize(obj["stepSize"].toDouble())
{
	QVariantMap tmpMap = obj["definitions"].toObject().toVariantMap();
	for (const auto & [key, value] : KeyVal(tmpMap)) {
		if (key.size() != 1) return;
		definitions[key[0].toLatin1()] = Definition(value.toJsonObject());
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

	QJsonObject jsonDefinitions;
	for (const auto & [key, value] : KeyVal(definitions)) {
		jsonDefinitions[QString(1, key)] = value.toJson();
	}
	rv["definitions"] = jsonDefinitions;

	return rv;
}

// ----------------------------------------------------------------------------

Definition::Definition(const QJsonObject & obj)
{
	color = obj["color"].toInt();
	command = obj["command"].toString();
	paint = obj["paint"].toBool();
}

Definition::Definition(const QString & command)
	: command(command),
	  paint(true)
{
}

QJsonObject Definition::toJson() const
{
	QJsonObject rv;
	rv["color"]   = (int)color;
	rv["command"] = command;
	rv["paint"]   = paint;
	return rv;
}

}
