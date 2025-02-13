#pragma once


// --------------------------------------------------------------------------
// enum class helpers
// --------------------------------------------------------------------------

template<typename T>
constexpr T AddToEnum(T value, int increment_value)
{
    return static_cast<T>(static_cast<int>(value) + increment_value);
}


template<typename T>
void IncrementEnum(T& value)
{
    static_assert(sizeof(T) == sizeof(int));
    ++reinterpret_cast<int&>(value);
}


template<typename T>
constexpr T FirstInEnum();


template<typename T>
constexpr T LastInEnum();
