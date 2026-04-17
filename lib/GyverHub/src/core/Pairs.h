#pragma once
#include <utility>

// Простая заглушка, если в библиотеке GyverPairs нет отдельного заголовка
// Обычно Pairs.h просто делает using namespace или подключает std::pair
// Если GyverHub ожидает специфичные макросы, их нужно добавить сюда.

// Часто в GyverLibs это просто алиас или обертка
template<typename K, typename V>
using Pair = std::pair<K, V>;

template<typename K, typename V>
Pair<K, V> makePair(const K& k, const V& v) {
    return std::make_pair(k, v);
}