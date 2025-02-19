#pragma once


// --------------------------------------------------------------------------
// some other useful templates
// --------------------------------------------------------------------------

#include <zToolsO/ConstReferenceOptional.h>
#include <zToolsO/PointerType.h>
#include <zToolsO/StandardTemplatesCpp20.h>
#include <zToolsO/VectorIterators.h>
#include <optional>



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


namespace cs
{
    // cs::is_optional
    template<typename T> struct is_optional                   : std::false_type {};
    template<typename T> struct is_optional<std::optional<T>> : std::true_type  {};
}
