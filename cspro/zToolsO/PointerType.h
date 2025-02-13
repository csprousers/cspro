#pragma once

#include <memory>


template<typename T> struct IsSharedPtr                                  : std::false_type {};
template<typename T> struct IsSharedPtr<std::shared_ptr<T>>              : std::true_type  {};
template<typename T> struct IsSharedPtr<std::shared_ptr<T>&>             : std::true_type  {};
template<typename T> struct IsSharedPtr<std::shared_ptr<const T>>        : std::true_type  {};
template<typename T> struct IsSharedPtr<std::shared_ptr<const T>&>       : std::true_type  {};
template<typename T> struct IsSharedPtr<const std::shared_ptr<T>>        : std::true_type  {};
template<typename T> struct IsSharedPtr<const std::shared_ptr<T>&>       : std::true_type  {};
template<typename T> struct IsSharedPtr<const std::shared_ptr<const T>>  : std::true_type  {};
template<typename T> struct IsSharedPtr<const std::shared_ptr<const T>&> : std::true_type  {};


template<typename T> struct IsUniquePtr                                  : std::false_type {};
template<typename T> struct IsUniquePtr<std::unique_ptr<T>>              : std::true_type  {};
template<typename T> struct IsUniquePtr<std::unique_ptr<T>&>             : std::true_type  {};
template<typename T> struct IsUniquePtr<std::unique_ptr<const T>>        : std::true_type  {};
template<typename T> struct IsUniquePtr<std::unique_ptr<const T>&>       : std::true_type  {};
template<typename T> struct IsUniquePtr<const std::unique_ptr<T>>        : std::true_type  {};
template<typename T> struct IsUniquePtr<const std::unique_ptr<T>&>       : std::true_type  {};
template<typename T> struct IsUniquePtr<const std::unique_ptr<const T>>  : std::true_type  {};
template<typename T> struct IsUniquePtr<const std::unique_ptr<const T>&> : std::true_type  {};


template<typename T>
constexpr bool IsPointer()
{
    return ( std::is_pointer_v<T> || IsSharedPtr<T>::value || IsUniquePtr<T>::value );
}


template<typename T>
constexpr auto GetPointer(T& pointer_or_value)
{
    if constexpr(std::is_pointer_v<T>)
    {
        return pointer_or_value;
    }

    else if constexpr(IsPointer<T>())
    {
        return pointer_or_value.get();
    }

    else
    {
        return &pointer_or_value;
    }
}
