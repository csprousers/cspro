#pragma once


// --------------------------------------------------------------------------
// some other useful templates
// --------------------------------------------------------------------------

#include <zToolsO/PointerType.h>
#include <zToolsO/VectorIterators.h>



// --------------------------------------------------------------------------
// a way to access the underlying value of a value, raw pointer, or a 
// shared pointer
// --------------------------------------------------------------------------

template<typename T> constexpr T& GetUnderlyingValue(T* const value_or_pointer)                  { return *value_or_pointer; }
template<typename T> constexpr T& GetUnderlyingValue(const std::shared_ptr<T>& value_or_pointer) { return *value_or_pointer; }
template<typename T> constexpr T& GetUnderlyingValue(const std::unique_ptr<T>& value_or_pointer) { return *value_or_pointer; }
template<typename T> constexpr T& GetUnderlyingValue(T& value_or_pointer)                        { return value_or_pointer;  }



// --------------------------------------------------------------------------
// optional value helpers
// --------------------------------------------------------------------------

template<typename T>
T ValueOrDefault(const std::optional<T>& value)
{
    if( value.has_value() )
        return *value;

    return T();
}

template<typename T>
T ValueOrDefault(std::optional<T>&& value)
{
    if( value.has_value() )
        return std::move(*value);

    return T();
}



// --------------------------------------------------------------------------
// C++20 functionality to be removed once we move on from C++17
// --------------------------------------------------------------------------

namespace std
{
    template<typename T>
    std::unique_ptr<T> make_unique_for_overwrite(const std::size_t size)
    {
        // from https://stackoverflow.com/questions/45703152/recommended-way-to-make-stdunique-ptr-of-array-type-without-value-initializati
        return unique_ptr<T>(new typename std::remove_extent<T>::type[size]);
    }
}


template<bool flag = false>
void static_assert_false()
{
    static_assert(flag);
}
