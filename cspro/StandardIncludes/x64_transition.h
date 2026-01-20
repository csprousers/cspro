#pragma once

#include <assert.h>


// --------------------------------------------------------------------------
// To support migrating the codebase to x64, the following casts are defined,
// with runtime asserts checking the validity of narrowed values.
//
// int32_cast: size_t -> int
//
// uint32_cast: size_t -> unsigned int
//
// string_len_cast<T> -> int for CString
//                    -> size_t otherwise
// --------------------------------------------------------------------------

#if INTPTR_MAX == INT64_MAX
#define X64_BUILD
constexpr bool IsX64() { return true; }
#else
constexpr bool IsX64() { return false; }
#endif


template<typename T>
int int32_cast(const T value)
{
    static_assert(sizeof(int) == 4 && sizeof(T) == ( IsX64() ? 8 : 4 ));
    assert(value == static_cast<int>(value));
    return static_cast<int>(value);
}


template<typename T>
unsigned int uint32_cast(const T value)
{
    static_assert(sizeof(unsigned int) == 4 && sizeof(T) == ( IsX64() ? 8 : 4 ));
    assert(value == static_cast<unsigned int>(value));
    return static_cast<unsigned int>(value);
}


template<typename T>
auto string_len_cast(const size_t value)
{
#ifdef USING_CSTRING
    if constexpr(std::is_same_v<T, CString>)
    {
        return int32_cast(value);
    }

    else
#endif
    {
        return value;
    }
}
