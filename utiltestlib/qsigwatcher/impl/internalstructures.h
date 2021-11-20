#pragma once

#include <QtCore>

#include <util/divutils.h>

namespace sigwatcher::impl {

class Location
{
public:
	Location() = default;
	Location(const QString & file, int line)
		: file(file), line(line)
	{}

	QString getFile() const { return file; }
	int getLine() const { return line; }

	QString getQtCreatorStr() const
	{
		// We need a newline and then exactly three spaces such that QtCreator creates a link
		return QString("\n   Loc: [%1(%2)]").arg(file).arg(line);
	}

private:
	QString file;
	int line = 0;
};

class ExpectOutput
{
public:
	ExpectOutput() = default;
	ExpectOutput(const Location & loc, const QString & expectStr)
		: loc(loc), expectStr(expectStr)
	{}

	QString getStrWithLoc() const
	{
		QString rv = loc.getQtCreatorStr();
		if (!expectStr.isNull() && showExtendedExpectation) {
			rv += "\n      " + expectStr;
		}
		return rv;
	}

public:
	static bool showExtendedExpectation;

private:
	Location loc;
	QString expectStr;
};

struct WaitingState
{
	bool isWaitingForExpected = false;
	bool isWaitingForUnexpected = false;
	QList<ExpectOutput> waitingList;
	Location location;
};

}
