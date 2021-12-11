#pragma once

#include <QtCore>

#include <cassert>
#include <tuple>
#include <type_traits>

#define CONCAT(a, b) _CONCAT2(a, b)
#define _CONCAT2(a, b) a ## b

#define TRUE_FUNC [](){ return true; }

#define METHOD_CHECKER(name, returnType, methodName, useConst) \
	template <typename C> \
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
	}; \
	\
	template<typename C> \
	inline constexpr bool CONCAT(name, _v) = name<C>::value;

#define UNUSED1(x) (void)(x)
#define UNUSED2(x,y) (void)(x),(void)(y)
#define UNUSED3(x,y,z) (void)(x),(void)(y),(void)(z)
#define UNUSED4(a,x,y,z) (void)(a),(void)(x),(void)(y),(void)(z)
#define UNUSED5(a,b,x,y,z) (void)(a),(void)(b),(void)(x),(void)(y),(void)(z)
#define UNUSED6(a,b,c,x,y,z) (void)(a),(void)(b),(void)(c),(void)(x),(void)(y),(void)(z)
#define VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5,_6,N,...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 6, 5, 4, 3, 2, 1)
#define ALL_UNUSED_IMPL_(nargs) UNUSED ## nargs
#define ALL_UNUSED_IMPL(nargs) ALL_UNUSED_IMPL_(nargs)
#define ALL_UNUSED(...) ALL_UNUSED_IMPL(VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__);

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

#define ENUM_OR(type) \
	inline type operator| (type a, type b) { return (type)((int)a | (int)b); }

inline void quitAndWait(const QList<QThread *> threads)
{
	for (QThread * thread : threads) thread->quit();
	for (QThread * thread : threads) thread->wait();
}

}

