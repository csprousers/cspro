#include "StdAfx.h"
#include "NumberToString.h"
#include "Special.h"


const std::string& GetIntToStringCache(const size_t value)
{
    static std::string cached_values[IntToStringCacheSize];
    ASSERT(value < IntToStringCacheSize);

    std::string& text = cached_values[value];

    if( text.empty() )
        text = IntToString<false>(value);

    return text;
}


std::string DoubleToString(const double value)
{
    if( IsSpecial(value) )
        return SpecialValues::ValueToString(value);

    char string_buffer[40];
    const int length = std::snprintf(string_buffer, _countof(string_buffer), "%0.6f", value);

    if( length <= 0 )
        return ReturnProgrammingError(std::string());

    const char* string_buffer_itr = string_buffer + length - 1;

    // right trim zeroes
    while( *string_buffer_itr == '0' )
        --string_buffer_itr;

    // don't display a decimal character if this is an integer
    if( *string_buffer_itr == '.' )
        --string_buffer_itr;

    return std::string(string_buffer, string_buffer_itr - string_buffer + 1);
}


std::string DoubleToString(double value, const std::optional<size_t>& min_decimals, const std::optional<size_t>& max_decimals)
{
    if( IsSpecial(value) )
        return SpecialValues::ValueToString(value);

    const size_t decimals_for_formatting = max_decimals.value_or(6);
    ASSERT(min_decimals.value_or(0) <= decimals_for_formatting);

    char string_buffer[40];
    const int length = std::snprintf(string_buffer, _countof(string_buffer), "%0.*f", static_cast<int>(decimals_for_formatting), value);

    const char* string_buffer_itr = string_buffer + length - 1;

    const char* final_decimal_pos = string_buffer_itr - decimals_for_formatting;
    ASSERT(*final_decimal_pos == '.');

    if( min_decimals.has_value() )
        final_decimal_pos += *min_decimals;

    // right trim zeroes
    while( string_buffer_itr > final_decimal_pos && *string_buffer_itr == '0' )
        --string_buffer_itr;

    // don't display a decimal character if this is an integer
    if( *string_buffer_itr == '.' )
        --string_buffer_itr;

    return std::string(string_buffer, string_buffer_itr - string_buffer + 1);
}
