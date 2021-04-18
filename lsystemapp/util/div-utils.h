#pragma once

#include <QtCore>

#define CONCAT(a, b) _CONCAT2(a, b)
#define _CONCAT2(a, b) a ## b

#define UNIQUE_NAME(base) CONCAT(base, __LINE__)

#define TRUE(cmd) [&](){ cmd; return true; }()

#define TRUE_FUNC [](){ return true; }

#define METHOD_CHECKER(name, returnType, methodName, useConst) \
	template <class C> \
	class name \
	{ \
		template <class T> \
		static std::true_type testSignature(returnType (T::*)() useConst); \
	\
		template <class T> \
		static decltype(testSignature(&T::methodName)) test(std::nullptr_t); \
	\
		template <class T> \
		static std::false_type test(...); \
	\
	public: \
		using type = decltype(test<C>(nullptr)); \
		static const bool value = type::value; \
	};

namespace util {

template<typename T>
QSharedPointer<T> ptr(const T & val) { return QSharedPointer<T>::create(val); }

template<typename T>
QSharedPointer<T> ptr(const QSharedPointer<T> & val) { return val; }

template<typename T>
T & ref(T & val) { return val; }

template<typename T>
T & ref(const QSharedPointer<T> & val) { return *val; }

template<typename T>
T & ref(QSharedPointer<T> & val) { return *val; }

}

