#pragma once

#include <util/print.h>

#include <algorithm>
#include <cmath>
#include <QTest>

// ------------- Comparisons of arbitrary objects with nicely formatted ouput -------------

#define PROCESS_QFAIL(tr, file, line) \
	if (!tr.success) { QTest::qFail((tr.message + util::test::impl::getTime()).toUtf8().data(), __FILE__, __LINE__); return; }

#define PROCESS_TESTRESULT(tr, file, line) \
	if (!tr.success) { \
		tr.message = QString("%1\n   Loc: [%2(%3)]").arg(tr.message.replace("\n", "\n   ")).arg(file).arg(line); \
		return tr; \
	}

#define CHECK_VERIFY(statement) \
	{ \
		auto tr = util::test::impl::verify((bool)statement, #statement); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define CHECK_VERIFY2(statement, description) \
	{ \
		auto tr = util::test::impl::verify((bool)statement, #statement, description); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define COMPARE(actual, expected) \
	{ \
		const auto tr = util::test::impl::generalCompare(actual, expected, #actual, #expected); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE(actual, expected) \
	{ \
		auto tr = util::test::impl::generalCompare(actual, expected, #actual, #expected); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define COMPARE_ADDR(actual, expected) \
	{ \
		const auto tr = util::test::impl::compareByAddress(actual, expected, #actual, #expected); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE_ADDR(actual, expected) \
	{ \
		auto tr = util::test::impl::compareByAddress(actual, expected, #actual, #expected); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define COMPARE_REGEXP(actual, expected) \
	{ \
		const auto tr = util::test::impl::regexCheck(actual, expected, #actual, #expected); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE_REGEXP(actual, expected) \
	{ \
		auto tr = util::test::impl::regexCheck(actual, expected, #actual, #expected); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define COMPARE_T(actual, ...) \
	{ \
		const auto tr = util::test::impl::generalCompareSameType(actual, __VA_ARGS__, #actual, #__VA_ARGS__); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE_T(actual, ...) \
	{ \
		const auto tr = util::test::impl::generalCompareSameType(actual, __VA_ARGS__, #actual, #__VA_ARGS__); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define COMPARE_C(actual, expected) \
	{ \
		const auto tr = util::test::impl::generalCompareCastToLeft(actual, expected, #actual, #expected); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE_C(actual, expected) \
	{ \
		auto tr = util::test::impl::generalCompareCastToLeft(actual, expected, #actual, #expected); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define COMPARE_INFO(actual, expected, infoVar) \
	{ \
		const auto tr = util::test::impl::generalCompareInfo(actual, expected, infoVar, #actual, #expected, #infoVar); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE_INFO(actual, expected, infoVar) \
	{ \
		auto tr = util::test::impl::generalCompareInfo(actual, expected, infoVar, #actual, #expected, #infoVar); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

namespace util::test::impl {

struct TestExpr
{
	TestExpr(const char * section, const char * varName, const QString & varContent)
		: section(section), varName(varName), varContent(varContent) {}
	QString section;
	QString varName;
	QString varContent;
};

struct TestResult
{
	TestResult() = default;
	explicit TestResult(bool success) : success(success) {}
	bool success = false;
	QString message;
};

TestResult verify(bool statement, const char * statementStr, const QString & description = QString());

QString getTestDiff(QList<TestExpr> testExpressions);
QString getTime();

template<typename T1, typename T2>
QString generateCompareFail(const T1 & lhs, const T2 & rhs, const char * lhsName, const char * rhsName)
{
	return "\n" + impl::getTestDiff({impl::TestExpr("Actual",   lhsName, print(lhs)),
									 impl::TestExpr("Expected", rhsName, print(rhs))});
}

template<typename T1, typename T2>
TestResult generalCompare(const T1 & lhs, const T2 & rhs, const char * lhsName, const char * rhsName, const QString & infoStr = QString())
{
	TestResult rv;
	rv.success = (lhs == (T1)rhs && (T2)lhs == rhs);
	if (!rv.success) {
		rv.message = "Compared values are not the same: "
				+ (infoStr.isEmpty() ? QString() : infoStr)
				+ generateCompareFail(lhs, rhs, lhsName, rhsName);
	}
	return rv;
}

template<typename T>
TestResult compareByAddress(const QSharedPointer<T> & lhs, const QSharedPointer<T> & rhs, const char * lhsName, const char * rhsName)
{
	TestResult rv;
	rv.success = ((uintptr_t)lhs.data() == (uintptr_t)rhs.data());
	if (!rv.success) rv.message = "Compared pointer addresses are not the same: " + generateCompareFail((uintptr_t)lhs.data(), (uintptr_t)rhs.data(), lhsName, rhsName);
	return rv;
}

template<typename T1>
TestResult generalCompare(const T1 & lhs, const char * rhs, const char * lhsName, const char * rhsName, const QString & infoStr = QString())
{
	return generalCompare(lhs, T1(rhs), lhsName, rhsName, infoStr);
}

template<typename T>
TestResult generalCompareSameType(const T & lhs, const T & rhs, const char * lhsName, const char * rhsName, const QString & infoStr = QString())
{
	return generalCompare<T,T>(lhs, rhs, lhsName, rhsName, infoStr);
}

template<typename T1, typename T2>
TestResult generalCompareCastToLeft(const T1 & lhs, const T2 & rhs, const char * lhsName, const char * rhsName, const QString & infoStr = QString())
{
	return generalCompare<T1,T1>(lhs, (T1)rhs, lhsName, rhsName, infoStr);
}

template<typename T1, typename T2, typename T3>
TestResult generalCompareInfo(const T1 & lhs, const T2 & rhs, const T3 & infoVal,
						const char * lhsName, const char * rhsName, const char * infoName)
{
	return generalCompare(lhs, rhs, lhsName, rhsName, QString("where %1 = %2").arg(infoName).arg(print(infoVal)));
}

// ------------- Fuzzy comparisons -------------

#define COMPARE_FUZZY(actual, expected) \
	{ \
		const auto tr = util::test::impl::fuzzyCompare(actual, expected, #actual, #expected); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE_FUZZY(actual, expected) \
	{ \
		auto tr = util::test::impl::fuzzyCompare(actual, expected, #actual, #expected); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define COMPARE_FUZZY_ACC(actual, expected, accuracy) \
	{ \
		const auto tr = util::test::impl::fuzzyCompareAcc(actual, expected, #actual, #expected, accuracy); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_COMPARE_FUZZY_ACC(actual, expected, accuracy) \
	{ \
		auto tr = util::test::impl::fuzzyCompareAcc(actual, expected, #actual, #expected, accuracy); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

template <class T>
TestResult fuzzyCompareAcc(T lhs, T rhs, const char * lhsName, const char * rhsName, const double accuracy)
{
	TestResult rv;

	// for accuracy = 0.01:
	// * 0 and x            for abs(x) < 0.02 are equal
	// * 1000 and (1000+x)  for abs(x) < 10   are equal
	const double diff = qAbs(lhs - rhs);
	const double maxDiff = qMax(accuracy, qMax(qAbs(lhs), qAbs(rhs)) * accuracy);
	rv.success = (diff <= maxDiff);
	if (!rv.success) {
		rv.message = QString("Compared values are not sufficiently similar (fuzzy compare with acc = %1, diff = %2, max allowed diff = %3):")
				.arg(accuracy).arg(diff).arg(maxDiff) + generateCompareFail(lhs, rhs, lhsName, rhsName);
	}
	return rv;
}

template <class T>
TestResult fuzzyCompare(T lhs, T rhs, const char * lhsName, const char * rhsName)
{
	const double Accuracy = 0.0001; // this accuracy should be ok for all cases
	return fuzzyCompareAcc(lhs, rhs, lhsName, rhsName, Accuracy);
}

TestResult regexCheck(const QString & actual, const QString & expected, const char * actualName, const char * expectedName);


// ------------- Set comparisons -------------


#define SET_COMPARE(set1, set2) \
	{ \
		const auto tr = util::test::impl::setCompare(set1, set2, #set1, #set2); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_SET_COMPARE(set1, set2) \
	{ \
		auto tr = util::test::impl::setCompare(set1, set2, #set1, #set2); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

template<typename T>
TestResult setCompare(const QSet<T> & lhs, const QSet<T> & rhs,
				const char * lhsName, const char * rhsName,
				bool showTime)
{
	TestResult rv;
	rv.success = (lhs == rhs);
	if (!rv.success) {
		QString & msg = rv.message;
		msg = QString("\n   Sets '%1' and '%2' are different!\n").arg(lhsName).arg(rhsName);
		if (rhs.contains(lhs)) {
			msg += QString("   The set '%1' is contained in '%2'\n").arg(lhsName).arg(rhsName);
		} else {
			msg += QString("   Diff '%1' - '%2' is: {%3}\n").arg(lhsName).arg(rhsName).arg(print(lhs - rhs));
		}
		if (lhs.contains(rhs)) {
			msg += QString("   The set '%1' is contained in '%2'\n").arg(rhsName).arg(lhsName);
		} else {
			msg += QString("   Diff '%1' - '%2' is: {%3}\n").arg(rhsName).arg(lhsName).arg(print(rhs - lhs));
		}
		if (showTime) msg += impl::getTime();
	}
	return rv;
}

#define CONTAINS(elem, set) \
	{ \
		const auto tr = util::test::impl::setContains(elem, set, #elem, #set); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CONTAINS_C(elem, set) \
	{ \
		const auto tr = util::test::impl::setContainsCastToSet(elem, set, #elem, #set); \
		PROCESS_QFAIL(tr, __FILE__, __LINE__) \
	}

#define CHECK_CONTAINS(elem, set) \
	{ \
		const auto tr = util::test::impl::setContains(elem, set, #elem, #set); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

#define CHECK_CONTAINS_C(elem, set) \
	{ \
		const auto tr = util::test::impl::setContainsCastToSet(elem, set, #elem, #set); \
		PROCESS_TESTRESULT(tr, __FILE__, __LINE__) \
	}

template<typename T>
TestResult setContains(const T & elem, const QSet<T> & set,
				 const char * elemName, const char * setName)
{
	TestResult rv;
	rv.success = set.contains(elem);
	if (!rv.success) {
		QString msg = "Searched element is not contained in set:\n"
				+ impl::getTestDiff({impl::TestExpr("Exp. elem.", elemName, print(elem)),
									 impl::TestExpr("Actual set", setName,  print(set))});
	}
	return rv;
}

template<typename T1, typename T2>
TestResult setContainsCastToSet(const T1 & elem, const QSet<T2> & set, const char * elemName, const char * setName)
{
	return setContains((T2)elem, set, elemName, setName);
}

} // namespace util::test::impl

