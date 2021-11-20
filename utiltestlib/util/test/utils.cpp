#include "utils.h"

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
	return printStr("\n   at: %1", QDateTime::currentDateTimeUtc());
}

TestResult regexCheck(const QString & actual, const QString & expected, const char * actualName, const char * expectedName)
{
	TestResult rv;
	rv.success = QRegularExpression(expected).match(actual).hasMatch();
	const QString expectedNameStr = expectedName;
	const QString useExpectedName = (expected == expectedNameStr.mid(1, expectedNameStr.length() - 2)) ? "<following regex>" : expected;
	if (!rv.success) {
		rv.message = "found no regular expression match:" + generateCompareFail(actual, expected, actualName, useExpectedName.toLatin1().data());
	}
	return rv;
}

TestResult verify(bool statement, const char * statementStr, const QString & description)
{
	TestResult rv;
	rv.success = statement;
	if (!rv.success) {
		rv.message = QString("'%1' returned FALSE. (%2)").arg(statementStr, description);
	}
	return rv;
}

}
