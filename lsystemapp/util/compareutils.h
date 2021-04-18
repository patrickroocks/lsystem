#pragma once

#include <util/print.h>

#include <algorithm>
#include <cmath>
#include <QTest>

// ------------- Comparisons of arbitrary objects with nicely formatted ouput -------------

#define COMPARE(actual, expected) \
	do {\
		if (!util::test::impl::generalCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) return;\
	} while (false)

#define COMPARE_PREPARE(actual, expected, prepare) \
do {\
	auto actualCopy = actual; \
	auto expectedCopy = expected; \
	(prepare)(actualCopy); \
	(prepare)(expectedCopy); \
	if (!util::test::impl::generalCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) return;\
} while (false)

#define COMPARE_T(actual, ...) \
	do {\
		if (!util::test::impl::generalCompareSameType(actual, __VA_ARGS__, #actual, #__VA_ARGS__, __FILE__, __LINE__)) return;\
	} while (false)

#define COMPARE_REGEXP(actual, expected) \
	do {\
		if (!util::test::impl::reStringCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) return;\
	} while (false)

#define COMPARE_FLOAT(actual, expected) \
	do {\
		if (!util::test::impl::doCompareFloat(actual, expected, #actual, #expected, __FILE__, __LINE__)) return; \
	} while (false)

#define COMPARE_XYZ(x1, y1, z1, x2, y2, z2) \
	do { \
		if (!util::test::impl::doCompareFloat(x1, x2, #x1, #x2, __FILE__, __LINE__)) return; \
		if (!util::test::impl::doCompareFloat(y1, y2, #y1, #y2, __FILE__, __LINE__)) return; \
		if (!util::test::impl::doCompareFloat(z1, z2, #z1, #z2, __FILE__, __LINE__)) return; \
	} while (false)

#define CHECK_COMPARE_XYZ(x1, y1, z1, x2, y2, z2) \
			   util::test::impl::doCompareFloat(x1, x2, #x1, #x2, __FILE__, __LINE__)  \
			&& util::test::impl::doCompareFloat(y1, y2, #y1, #y2, __FILE__, __LINE__)  \
			&& util::test::impl::doCompareFloat(z1, z2, #z1, #z2, __FILE__, __LINE__)

#define COMPARE_C(actual, expected) \
	do {\
		if (!util::test::impl::generalCompareCastToLeft(actual, expected, #actual, #expected, __FILE__, __LINE__)) return;\
	} while (false)

#define COMPARE_INFO(actual, expected, infoVar) \
	do {\
		if (!util::test::impl::generalCompareInfo(actual, expected, infoVar, #actual, #expected, #infoVar, __FILE__, __LINE__)) return;\
	} while (false)

namespace util::test {

namespace impl {

struct TestExpr {
	TestExpr(const char * section, const char * varName, const QString & varContent)
		: section(section), varName(varName), varContent(varContent) {}
	QString section;
	QString varName;
	QString varContent;
};

QString getTestDiff(QList<TestExpr> testExpressions);
QString getTime();

template<typename T1, typename T2>
void generateCompareFail(const T1 & lhs, const T2 & rhs, const char * lhsName, const char * rhsName, const char * file, int line, const QString & infoStr = QString())
{
	QString msg = "Compared values are not the same:";
	if (!infoStr.isEmpty()) msg += " (" + infoStr + ")";
	msg += "\n" + impl::getTestDiff({impl::TestExpr("Actual",   lhsName, print(lhs)),
									 impl::TestExpr("Expected", rhsName, print(rhs))});
	msg += impl::getTime();
	QTest::qFail(msg.toUtf8().data(), file, line);
}

template<typename T1, typename T2>
bool generalCompare(const T1 & lhs, const T2 & rhs, const char * lhsName, const char * rhsName, const char * file, int line, const QString & infoStr = QString())
{
	if (lhs != (T1)rhs || (T2)lhs != rhs) {
		generateCompareFail(lhs, rhs, lhsName, rhsName, file, line, infoStr);
		return false;
	}
	return true;
}

template<typename T1>
bool generalCompare(const T1 & lhs, const char * rhs, const char * lhsName, const char * rhsName, const char * file, int line, const QString & infoStr = QString())
{
	return generalCompare(lhs, T1(rhs), lhsName, rhsName, file, line, infoStr);
}

template<typename T>
bool generalCompareSameType(const T & lhs, const T & rhs, const char * lhsName, const char * rhsName, const char * file, int line, const QString & infoStr = "")
{
	return generalCompare<T,T>(lhs, rhs, lhsName, rhsName, file, line, infoStr);
}

template<typename T1, typename T2>
bool generalCompareCastToLeft(const T1 & lhs, const T2 & rhs, const char * lhsName, const char * rhsName, const char * file, int line, const QString & infoStr = "")
{
	return generalCompare<T1,T1>(lhs, (T1)rhs, lhsName, rhsName, file, line, infoStr);
}

template<typename T1, typename T2, typename T3>
bool generalCompareInfo(const T1 & lhs, const T2 & rhs, const T3 & infoVal,
						const char * lhsName, const char * rhsName, const char * infoName, const char * file, int line)
{
	return generalCompare(lhs, rhs, lhsName, rhsName, file, line, QString("where %1 = %2").arg(infoName).arg(print(infoVal)));
}

template<class A, class E>
bool doCompareFloat(A actual, E expected, const char * lhsName, const char * rhsName, const char * file, int line)
{
	static_assert(std::is_floating_point<A>::value, "the arguments of COMPARE_FLOAT should be double. Or use QCOMPARE instead"); \
	static_assert(std::is_floating_point<E>::value, "the arguments of COMPARE_FLOAT should be double. Or use QCOMPARE instead"); \

	return generalCompareSameType((float)actual, (float)expected, lhsName, rhsName, file, line);
}

// ------------- Fuzzy comparisons -------------

#define COMPARE_FUZZY(actual, expected) \
	do {\
		if (!util::test::impl::fuzzyCompare(actual, expected, #actual, #expected, __FILE__, __LINE__)) return;\
	} while (false)

#define COMPARE_FUZZY_ACC(actual, expected, accuracy) \
	do {\
		if (!util::test::impl::fuzzyCompareAcc(actual, expected, #actual, #expected, __FILE__, __LINE__, accuracy)) return;\
	} while (false)

#define CHECK_COMPARE_FUZZY_ACC(actual, expected, accuracy) \
	util::test::impl::fuzzyCompareAcc(actual, expected, #actual, #expected, __FILE__, __LINE__, accuracy)

template <class T>
bool fuzzyCompareAcc(T lhs, T rhs, const char * lhsName, const char * rhsName, const char * file, int line, const double accuracy)
{
	// for accuracy = 0.01:
	// * 0 and x            for abs(x) < 0.02 are equal
	// * 1000 and (1000+x)  for abs(x) < 10   are equal
	const double diff = qAbs(lhs - rhs);
	const double maxDiff = qMax(accuracy, qMax(qAbs(lhs), qAbs(rhs)) * accuracy);
	if (diff <= maxDiff) return true;
	generateCompareFail(lhs, rhs, lhsName, rhsName, file, line,
			QString("fuzzy compare with acc = %1, diff = %2, max allowed diff = %3").arg(accuracy).arg(diff).arg(maxDiff));
	return false;
}

template <class T>
bool fuzzyCompare(T lhs, T rhs, const char * lhsName, const char * rhsName, const char * file, int line)
{
	const double Accuracy = 0.0001; // this accuracy should be ok for all cases
	return fuzzyCompareAcc(lhs, rhs, lhsName, rhsName, file, line, Accuracy);
}

bool reStringCompare(const QString & actual, const QString & expected, const char * actualName, const char * expectedName, const char * file, int line);

// ------------- Fuzzy comparisons for  -------------

#define COMPARE_FUZZY_XYZ_MEMBERS(actual, expected) \
	do { \
		if (!util::test::impl::fuzzyCompare(actual.x, expected.x, QByteArray(#actual) + ".x", QByteArray(#expected) + ".x", __FILE__, __LINE__)) return; \
		if (!util::test::impl::fuzzyCompare(actual.y, expected.y, QByteArray(#actual) + ".y", QByteArray(#expected) + ".y", __FILE__, __LINE__)) return; \
		if (!util::test::impl::fuzzyCompare(actual.z, expected.z, QByteArray(#actual) + ".z", QByteArray(#expected) + ".z", __FILE__, __LINE__)) return; \
	} while (false)

#define COMPARE_FUZZY_ABC_MEMBERS(actual, expected) \
	do { \
		if (!util::test::impl::fuzzyCompare(actual.a, expected.a, QByteArray(#actual) + ".a", QByteArray(#expected) + ".a", __FILE__, __LINE__)) return; \
		if (!util::test::impl::fuzzyCompare(actual.b, expected.b, QByteArray(#actual) + ".b", QByteArray(#expected) + ".b", __FILE__, __LINE__)) return; \
		if (!util::test::impl::fuzzyCompare(actual.c, expected.c, QByteArray(#actual) + ".c", QByteArray(#expected) + ".c", __FILE__, __LINE__)) return; \
	} while (false)

} // namespace impl

// ------------- Sorted comparisons -------------

// compare functions for std::sort
namespace impl {

template <typename T>
bool byId(const T & lhs, const T & rhs) { return lhs.id < rhs.id; }

} // namespace impl

template <typename T>
QList<T> sortedById(const QList<T> & list)
{
	QList<T> rv = list;
	std::sort(rv.begin(), rv.end(), &impl::byId<T>);
	return rv;
}

// ------------- Set comparisons -------------


#define SET_COMPARE(set1, set2) \
	do {\
		if (!util::test::impl::setCompare(set1, set2, #set1, #set2, __FILE__, __LINE__)) return;\
	} while (false)

namespace impl {

template<typename T>
bool setCompare(const QSet<T> & lhs, const QSet<T> & rhs,
				const char * lhsName, const char * rhsName,
				const char * file, int line)
{
	if (lhs == rhs) return true;
	QString msg = QString("\n   Sets '%1' and '%2' are different!\n").arg(lhsName).arg(rhsName);
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
	msg += impl::getTime();
	QTest::qFail(msg.toUtf8().data(), file, line);
	return false;
}

#define CONTAINS(elem, set) \
	do {\
		if (!util::test::impl::setContains(elem, set, #elem, #set, __FILE__, __LINE__)) return;\
	} while (false)

#define CONTAINS_C(elem, set) \
	do {\
		if (!util::test::impl::setContainsCastToSet(elem, set, #elem, #set, __FILE__, __LINE__)) return;\
	} while (false)

template<typename T>
bool setContains(const T & elem, const QSet<T> & set,
				 const char * elemName, const char * setName,
				 const char * file, int line)
{
	if (set.contains(elem)) return true;
	QString msg = "Searched element is not contained in set:\n"
			+ impl::getTestDiff({impl::TestExpr("Exp. elem.", elemName, print(elem)),
								 impl::TestExpr("Actual set", setName,  print(set))})
			+ impl::getTime();
	QTest::qFail(msg.toUtf8().data(), file, line);
	return false;
}

template<typename T1, typename T2>
bool setContainsCastToSet(const T1 & elem, const QSet<T2> & set, const char * elemName, const char * setName, const char * file, int line)
{
	return setContains((T2)elem, set, elemName, setName, file, line);
}

} // namespace impl

}
