#pragma once

#include <util/divutils.h>
#include <util/qtcontutils.h>

#include <deque>

namespace util {

METHOD_CHECKER(has_to_string, QString, toString,  const)

namespace impl {

// * Fundamentals

// numbers
template<typename T>
inline typename std::enable_if_t<std::is_arithmetic_v<T>, QString>
printImpl(const T & val) { return QString::number(val); }

// bool
inline QString printImpl(const bool & val) { return val ? "true" : "false"; }

// strings
inline QString printImpl(const char        & val) { return QString(QChar(val)); }
inline QString printImpl(const QChar       & val) { return QString(val);        }
inline QString printImpl(const QString     & val) { return val;                 }
inline QString printImpl(const QByteArray  & val) { return QString(val);        }
inline QString printImpl(const char * chars)      { return QString(chars);      }

// * Qt Types
inline QString printImpl(const QDateTime & dateTime) { return dateTime.isNull() ? "<nulldate>" : dateTime.toString(Qt::ISODateWithMs); }
inline QString printImpl(const QUrl      & url)      { return url.toString(); }
inline QString printImpl(const QPoint    & point)    { return QString("(%1, %2)").arg(point.x()).arg(point.y()); }
inline QString printImpl(const QPointF   & point)    { return QString("(%1, %2)").arg(point.x()).arg(point.y()); }
inline QString printImpl(const QFileInfo & fileInfo) { return fileInfo.absoluteFilePath(); }

// for flags we use binary encoding as there we see the flags which are set quite easy
template<typename T>
inline QString printImpl(const QFlags<T> & flags) { return QString("0b%1").arg((quint64)flags, 0, 2); }

// * // Forward declarations for nestable type

template<typename T>               QString printImpl(const QSharedPointer<T> & cl);
template<typename T>               QString printImpl(const QScopedPointer<T> & cl);
template<typename T>               QString printImpl(      T*                  cl);
template<typename T1, typename T2> QString printImpl(const QPair<T1,T2>     & val);
template<typename K,  typename V>  QString printImpl(const QMap      <K, V> & map);
template<typename K,  typename V>  QString printImpl(const QMultiMap <K, V> & map);
template<typename K,  typename V>  QString printImpl(const QHash     <K, V> & hash);
template<typename K,  typename V>  QString printImpl(const QMultiHash<K, V> & hash);
template<typename T>               QString printImpl(const QList<T>         & lst);
template<typename T>               QString printImpl(const QSet<T>          & set);
template<typename T>               QString printImpl(const std::list<T>     & lst);
template<typename T>               QString printImpl(const std::vector<T>   & vec);
template<typename... T>            QString printImpl(const std::tuple<T...> & tup);
template<typename T, typename A>   QString printImpl(const std::deque<T, A> & deq);

// * search for toString
template<typename T>
inline typename std::enable_if_t<has_to_string_v<T> && !std::is_enum_v<T>, QString>
printImpl(const T & cl) { return cl.toString(); }

// * check if is enum with Q_ENUM macro
template<typename T>
inline typename std::enable_if_t<!has_to_string_v<T> && std::is_enum_v<T>, QString>
printImpl(const T & cl) { return QVariant::fromValue(cl).toString(); }

// * other classes: assume that () operator is overloaded (std::function, lambda, ...)
template<typename T>
inline typename std::enable_if_t<!has_to_string_v<T> && !std::is_enum_v<T> && !std::is_arithmetic_v<T>, QString>
printImpl(const T & fun) { return fun(); }

// * Containers and complex types definitions (recursion possible!)

// * pointer implementations

template<typename T>
QString printImpl(const QSharedPointer<T> & cl) { return cl ? impl::printImpl(*cl) : "<null>"; }

template<typename T>
QString printImpl(const QScopedPointer<T> & cl) { return cl ? impl::printImpl(*cl) : "<null>"; }

template<typename T>
QString printImpl(T* cl) { return cl ? impl::printImpl(*cl) : "<null>"; }

inline QString printImpl(std::nullptr_t) { return "<null>"; }

// * Containers and complex types implementations

template<typename T1, typename T2>
QString printImpl(const QPair<T1,T2> & val)
{
	return QString("(%1, %2)").arg(printImpl(val.first))
							 .arg(printImpl(val.second));
}

template<typename Container>
QString printMapping(const Container & cont)
{
	QString rv('[');
	typename q_container_traits<Container>::const_iterator it(cont);
	bool isFirst = true;
	while (it.hasNext()) {
		it.next();
		if (isFirst) isFirst = false;
		else rv += ", ";
		rv += printImpl(it.key()) + ':' + printImpl(it.value());
	}
	rv += ']';
	return rv;
}

template<typename K, typename V> QString printImpl(const QMap      <K, V> & map ) { return printMapping(map ); }
template<typename K, typename V> QString printImpl(const QMultiMap <K, V> & map ) { return printMapping(map ); }
template<typename K, typename V> QString printImpl(const QHash     <K, V> & hash) { return printMapping(hash); }
template<typename K, typename V> QString printImpl(const QMultiHash<K, V> & hash) { return printMapping(hash); }

template<typename Container>
QString printCollection(const Container & cont)
{
	QString rv;
	bool isFirst = true;
	for (const auto & elem : cont) {
		if (isFirst) isFirst = false;
		else rv += ", ";
		rv += printImpl(elem);
	}
	return rv;
}

template<typename T>             QString printImpl(const QList      <T>    & vec) { return "(" + printCollection(vec) + ")"; }
template<typename T>             QString printImpl(const QSet       <T>    & set) { return "{" + printCollection(set) + "}"; }

template<typename T>             QString printImpl(const std::list  <T>    & lst) { return "[" + printCollection(lst) + "]"; }
template<typename T>             QString printImpl(const std::vector<T>    & vec) { return "[" + printCollection(vec) + "]"; }
template<typename T, typename A> QString printImpl(const std::deque <T, A> & deq) { return "[" + printCollection(deq) + "]"; }

// * tuples

template<typename T, T...> struct integer_sequence {};
template<std::size_t N, std::size_t... I> struct gen_indices : gen_indices<(N - 1), (N - 1), I...> {};
template<std::size_t... I> struct gen_indices<0, I...> : integer_sequence<std::size_t, I...> {};

template<typename H>
QString & printTupleImpl(QString & s, H && h)
{
	s += printImpl(std::forward<H>(h));
	return s;
}

template<typename H, typename... T>
QString & printTupleImpl(QString & s, H && h, T &&... t)
{
	s += printImpl(std::forward<H>(h)) + ", ";
	return printTupleImpl(s, std::forward<T>(t)...);
}

template<typename... T, std::size_t... I>
QString printImpl(const std::tuple<T...> & tup, integer_sequence<std::size_t, I...>)
{
	QString rv;
	int ctx[] = { (printTupleImpl(rv, std::get<I>(tup)...), 0), 0 };
	(void)ctx;
	return "(" + rv + ")";
}

template<typename... T>
QString printImpl(const std::tuple<T...> & tup)
{
	// https://stackoverflow.com/questions/23436406/converting-tuple-to-string
	return printImpl(tup, gen_indices<sizeof...(T)>{});
}

} // end namespace impl, now in util

template<typename T>
inline QString print(const T & cl) { return impl::printImpl(cl); }

template<typename... Params>
static QString printStr(const char * str, const Params & ... params)
{
	if constexpr (sizeof...(Params) == 0) {
		return str;
	} else {
		using TupleType = std::tuple<const Params & ...>;
		TupleType t = std::forward_as_tuple(params...); // doesn't cause a copy, still a reference!

		constexpr const size_t ts = std::tuple_size<TupleType>::value;
		static_assert(ts <= 19, "currently only 19 parameters for printStr(...) are allowed");

		QString msg;
		msg.reserve(128);

		const char * p = str;
		while (*p) {
			if (*p == '%') {
				const qint8 i = *(p+1) - '0';
				if (ts > 9 && i == 1) {
					msg += QByteArray(str, (int)(p - str));
					const qint8 j = *(p+2) - '0';
					if (j >= 0 && j <= 9 && 10 + j <= (qint8)ts) {
						switch (j) {
							case 0: msg += print(std::get< 9 < ts ?  9 : 0>(t)); break;
							case 1: msg += print(std::get<10 < ts ? 10 : 0>(t)); break;
							case 2: msg += print(std::get<11 < ts ? 11 : 0>(t)); break;
							case 3: msg += print(std::get<12 < ts ? 12 : 0>(t)); break;
							case 4: msg += print(std::get<13 < ts ? 13 : 0>(t)); break;
							case 5: msg += print(std::get<14 < ts ? 14 : 0>(t)); break;
							case 6: msg += print(std::get<15 < ts ? 15 : 0>(t)); break;
							case 7: msg += print(std::get<16 < ts ? 16 : 0>(t)); break;
							case 8: msg += print(std::get<17 < ts ? 17 : 0>(t)); break;
							case 9: msg += print(std::get<18 < ts ? 18 : 0>(t)); break;
						}
						p += 3; str = p;
						continue;
					} else {
						msg += print(std::get<0>(t));
						p += 2; str = p;
						continue;
					}
				} else if (i >= 1 && i <= (qint8)ts) {
					msg += QByteArray(str, (int)(p - str));
					switch (i) {
						case 1: msg += print(std::get<0             >(t)); break;
						case 2: msg += print(std::get<1 < ts ? 1 : 0>(t)); break;
						case 3: msg += print(std::get<2 < ts ? 2 : 0>(t)); break;
						case 4: msg += print(std::get<3 < ts ? 3 : 0>(t)); break;
						case 5: msg += print(std::get<4 < ts ? 4 : 0>(t)); break;
						case 6: msg += print(std::get<5 < ts ? 5 : 0>(t)); break;
						case 7: msg += print(std::get<6 < ts ? 6 : 0>(t)); break;
						case 8: msg += print(std::get<7 < ts ? 7 : 0>(t)); break;
						case 9: msg += print(std::get<8 < ts ? 8 : 0>(t)); break;
					}
					p += 2; str = p;
					continue;
				}
			}
			++p;
		}
		if (str != p) msg += QByteArray(str, (int)(p - str));

		return msg;
	}
}

}

