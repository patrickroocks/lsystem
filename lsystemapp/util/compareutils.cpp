#include "compareutils.h"

namespace util::test::impl {

QString getTestDiff(QList<TestExpr> testExpressions)
{
	// formatted name: "   SECTION (VARNAME): " (length = length(section) + length(varname) + spaces + 8)

	auto shorten = [](const QString & str) -> QString {
		return str.size() > 30 ? (str.left(27) + "...") : str;
	};

	int sectionMaxSize = 0;
	int varMaxSize = 0;

	for (TestExpr & testExpr : testExpressions) {
		testExpr.varName = shorten(testExpr.varName);
		sectionMaxSize = qMax(sectionMaxSize, testExpr.section.size());
		varMaxSize     = qMax(varMaxSize,     testExpr.varName.size());
	}

	const int indent = sectionMaxSize + varMaxSize + 8;
	const int lineWidth = qMax(80, 140 - indent);

	auto insertNewlinesAndSpace = [&](const QString & str) {
		if (str.size() < lineWidth) return str;
		QString res;
		int curPos = 0;
		bool firstLine = true;
		while (true) {
			QString nextLine = str.mid(curPos, lineWidth);
			if (nextLine.isEmpty()) return res;
			for (int i = nextLine.length() - 1 ; i > 40 ; i--) {
				const QCharRef c = nextLine[i];
				if (c == ',' || c == ':' || c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']') {
					nextLine = nextLine.left(i + 1);
					break;
				}
			}
			curPos += nextLine.size();
			if (!firstLine) res += "\n" + QString(indent, ' ');
			firstLine = false;
			res += nextLine;
		}
	};

	QString rv;
	for (const TestExpr & testExpr : testExpressions) {
		if (!rv.isEmpty()) rv += "\n";
		QString formattedName = QString("   ") + testExpr.section +                QString(sectionMaxSize - testExpr.section.length(), ' ')
							  + QString(" (")  + testExpr.varName + QString(")") + QString(varMaxSize     - testExpr.varName.length(), ' ')
							  + QString(": ");
		rv += formattedName + insertNewlinesAndSpace(testExpr.varContent);
	}

	return rv;
}

QString getTime()
{
	return QString("\n   at: ") + QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

bool reStringCompare(const QString & actual, const QString & expected, const char * actualName, const char * expectedName, const char * file, int line)
{
	if (QRegularExpression(expected).match(actual).hasMatch()) return true;
	generateCompareFail(actual, expected, actualName, expectedName, file, line, "found no regular expression match, forgot to escape special symbols?");
	return false;
}

}
