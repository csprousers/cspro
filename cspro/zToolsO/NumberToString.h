#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/TextFormatter.h>
#include <inttypes.h>


// --------------------------------------------------------------------------
// integer -> string
// --------------------------------------------------------------------------

#define Formatter_int64_t  _T("%") PRId64
#define Formatter_uint64_t _T("%") PRIu64

template<bool UseCache = true, typename T>
CString IntToString(T value);

constexpr int IntToStringLength(int value);


// --------------------------------------------------------------------------
// double -> string
// --------------------------------------------------------------------------

CLASS_DECL_ZTOOLSO std::wstring DoubleToString(double value);
CLASS_DECL_ZTOOLSO std::wstring DoubleToString(double value, const std::optional<size_t>& min_decimals, const std::optional<size_t>& max_decimals);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr size_t IntToStringCacheSize = 100;
CLASS_DECL_ZTOOLSO const CString& GetIntToStringCache(size_t value);


template<bool UseCache/* = true*/, typename T>
CString IntToString(T value)
{
    if constexpr(UseCache)
    {
        if constexpr(std::is_same_v<T, bool>)
        {
            return GetIntToStringCache(static_cast<size_t>(value));
        }

        else
        {
            if( value >= static_cast<T>(0) && value < static_cast<T>(IntToStringCacheSize) )
                return GetIntToStringCache(static_cast<size_t>(value));
        }
    }

    // 32-bit ints
    if constexpr(( std::is_same_v<T, bool> && !UseCache ) ||
                 ( std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t> ) ||
                 ( std::is_same_v<T, int16_t> || std::is_same_v<T, uint16_t> ) ||
                 ( std::is_same_v<T, int32_t> ))
    {
        return FormatText(_T("%") PRId32, static_cast<int32_t>(value));
    }

    // 32-bit unsigneds
#ifdef WASM
    else if constexpr(std::is_same_v<T, uint32_t> ||
                      std::is_same_v<T, unsigned long>)
#else
    else if constexpr(std::is_same_v<T, uint32_t>)
#endif    
    {
        static_assert(sizeof(T) == sizeof(uint32_t));
        return FormatText(_T("%") PRIu32, static_cast<uint32_t>(value));
    }

    // 64-bit ints
    else if constexpr(std::is_same_v<T, int64_t>)
    {
        return FormatText(Formatter_int64_t, value);
    }

    // 64-bit unsigned
    else if constexpr(!std::is_same_v<T, bool>)
    {
        if constexpr(std::is_same_v<T, uint64_t>)
        {
            return FormatText(Formatter_uint64_t, value);
        }

        else
        {
            static_assert_false();
        }
    }
}


constexpr int IntToStringLength(int value)
{
    if( value < 0 ) return 1 + IntToStringLength(( value == INT_MIN ) ? INT_MAX : -value);
    if( value < 10 ) return 1;
    if( value < 100 ) return 2;
    if( value < 1000 ) return 3;
    if( value < 10000 ) return 4;
    if( value < 100000 ) return 5;
    if( value < 1000000 ) return 6;
    if( value < 10000000 ) return 7;
    if( value < 100000000 ) return 8;
    if( value < 1000000000 ) return 9;

    static_assert(( static_cast<double>(INT_MAX) / 10000000000 ) < 1);
    return 10;
}
