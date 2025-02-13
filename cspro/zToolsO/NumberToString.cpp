#include "StdAfx.h"
#include "NumberToString.h"
#include "Special.h"


const CString& GetIntToStringCache(size_t value)
{
    static CString cached_values[IntToStringCacheSize];
    ASSERT(value < IntToStringCacheSize);

    CString& text = cached_values[value];

    if( text.IsEmpty() )
        text = IntToString<false>(value);

    return text;
}


std::wstring DoubleToString(double value)
{
    if( IsSpecial(value) )
        return SpecialValues::ValueToString(value);

    TCHAR string_buffer[80];
    int length = _stprintf(string_buffer, _T("%0.6f"), value);

    if( length <= 0 )
        return ReturnProgrammingError(std::wstring());

    TCHAR* string_buffer_itr = string_buffer + length - 1;

    // right trim zeroes
    while( *string_buffer_itr == '0' )
        --string_buffer_itr;

    // don't display a decimal character if this is an integer
    if( *string_buffer_itr == '.' )
        --string_buffer_itr;

    return std::wstring(string_buffer, string_buffer_itr - string_buffer + 1);
}


std::wstring DoubleToString(double value, const std::optional<size_t>& min_decimals, const std::optional<size_t>& max_decimals)
{
    if( IsSpecial(value) )
        return SpecialValues::ValueToString(value);

    size_t decimals_for_formatting = max_decimals.value_or(6);
    ASSERT(min_decimals.value_or(0) <= decimals_for_formatting);

    TCHAR string_buffer[80];
    int length = _stprintf(string_buffer, _T("%0.*f"), (int)decimals_for_formatting, value);

    TCHAR* string_buffer_itr = string_buffer + length - 1;

    const TCHAR* final_decimal_pos = string_buffer_itr - decimals_for_formatting;
    ASSERT(*final_decimal_pos == '.');

    if( min_decimals.has_value() )
        final_decimal_pos += *min_decimals;

    // right trim zeroes
    while( string_buffer_itr > final_decimal_pos && *string_buffer_itr == '0' )
        --string_buffer_itr;

    // don't display a decimal character if this is an integer
    if( *string_buffer_itr == '.' )
        --string_buffer_itr;

    return std::wstring(string_buffer, string_buffer_itr - string_buffer + 1);
}
