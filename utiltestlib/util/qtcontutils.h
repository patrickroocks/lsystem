#pragma once
#include <QtCore>

namespace util {

template<class Container> class KeyVal
{
public:
	KeyVal(const Container & container) : c(container) {}
	typename Container::const_key_value_iterator begin() const { return c.constKeyValueBegin(); }
	typename Container::const_key_value_iterator end() const { return c.constKeyValueEnd(); }
private:
	const Container & c;
};

template <typename T> struct inner_type
{
	using type = T;
};

template <typename T> struct inner_type<QSharedPointer<T>>
{
	using type = T;
};

template <typename T> struct inner_type<const QSharedPointer<T>>
{
	using type = T;
};

template <typename T> struct ref_type
{
	using type = T &;
};

template <typename T> struct ref_type<QSharedPointer<T>>
{
	using type = const QSharedPointer<T> &;
};

template <typename T> struct ref_type<const QSharedPointer<T>>
{
	using type = const QSharedPointer<T> &;
};

// Extract iterators / inner type from QT Container Types
// https://stackoverflow.com/questions/54232760/qt-construct-a-mutable-iterator-for-template-maps-lists-sets/54233956#54233956
template <typename Container> struct q_container_traits;

template <typename T> struct q_container_traits<QList<T>>
{
	using value_type = T;
	using value_inner_type = typename inner_type<T>::type;
	using mutable_iterator = QMutableListIterator<T>;
	using const_iterator = QListIterator<T>;
};

template <typename Key, typename Value> struct q_container_traits<QMap<Key, Value>>
{
	using value_type = Value;
	using value_inner_type = typename inner_type<Value>::type;
	using key_type = Key;
	using mutable_iterator = QMutableMapIterator<Key, Value>;
	using const_iterator = QMapIterator<Key, Value>;
};

template <typename Key, typename Value> struct q_container_traits<QMultiMap<Key, Value>>
{
	using value_type = Value;
	using value_inner_type = typename inner_type<Value>::type;
	using key_type = Key;
	using mutable_iterator = QMutableMapIterator<Key, Value>;
	using const_iterator = QMapIterator<Key, Value>;
};

template <typename Key, typename Value> struct q_container_traits<QHash<Key, Value>>
{
	using value_type = Value;
	using value_inner_type = typename inner_type<Value>::type;
	using key_type = Key;
	using mutable_iterator = QMutableHashIterator<Key, Value>;
	using const_iterator = QHashIterator<Key, Value>;
};

template <typename Key, typename Value> struct q_container_traits<QMultiHash<Key, Value>>
{
	using value_type = Value;
	using value_inner_type = typename inner_type<Value>::type;
	using key_type = Key;
	using mutable_iterator = QMutableHashIterator<Key, Value>;
	using const_iterator = QHashIterator<Key, Value>;
};

}
