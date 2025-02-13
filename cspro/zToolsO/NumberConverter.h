#pragma once
#include <zToolsO/Special.h>

namespace NumberConverter
{
    // TEXT -> NUMBER
    inline void AdvancePastSpaces(const TCHAR*& text, const TCHAR* text_end)
    {
        for( ; text < text_end && *text == _T(' '); ++text )
        {
        }
    }


    // if a decimal character is not found, then number_decimals indicates how
    // many decimal characters should be used
    inline double TextToDouble(const TCHAR* text, size_t text_length, size_t number_decimals)
    {
        const TCHAR* text_end = text + text_length;
        AdvancePastSpaces(text, text_end);

        // an empty string is notappl
        if( text == text_end )
            return NOTAPPL;

        // allow a sign, and then blanks after the sign
        bool negative = ( *text == _T('-') );

        if( negative || *text == _T('+') )
        {
            AdvancePastSpaces(++text, text_end);

            // if a sign but no value, then an invalid value
            if( text == text_end )
                return DEFAULT;
        }

        // now process the integer part of the number
        ASSERT(text < text_end);

        double value = 0;
        bool decimal_mark_found = false;

        for( ; text < text_end; ++text )
        {
            // allow periods and commas as the decimal mark
            if( *text == _T('.') || *text == _T(',') )
            {
                decimal_mark_found = true;
                ++text;
                break;
            }

            char16_t digit_value = ( *text - _T('0') );

            // any value other than a number is invalid
            if( digit_value > 9 )
                return DEFAULT;

            value = value * 10 + digit_value;
        }

        // if no decimal mark, adjust to account for the implied decimal mark
        if( !decimal_mark_found )
            value /= Power10[number_decimals];

        // otherwise process the fraction part of the number
        else
        {
            double fraction_part = 0;
            size_t explicit_number_decimals = 0;

            for( ; text < text_end; ++text )
            {
                char16_t digit_value = ( *text - _T('0') );

                // any value other than a number is invalid
                if( digit_value > 9 )
                    return DEFAULT;

                fraction_part = fraction_part * 10 + digit_value;
                ++explicit_number_decimals;
            }

            value += fraction_part / Power10[explicit_number_decimals];
        }

        ASSERT(text == text_end);

        return (double)( negative ? ( -1 * value ) : value );
    }


    template<typename T>
    inline double TextToIntegerDouble(const TCHAR* text, size_t text_length)
    {
        const TCHAR* text_end = text + text_length;
        AdvancePastSpaces(text, text_end);

        // an empty string is notappl
        if( text == text_end )
            return NOTAPPL;

        // allow a sign, and then blanks after the sign
        bool negative = ( *text == _T('-') );

        if( negative || *text == _T('+') )
        {
            AdvancePastSpaces(++text, text_end);

            // if a sign but no value, then an invalid value
            if( text == text_end )
                return DEFAULT;
        }

        // now process the number
        ASSERT(text < text_end);

        T value = 0;

        for( ; text < text_end; ++text )
        {
            char16_t digit_value = ( *text - _T('0') );

            // any value other than a number is invalid
            if( digit_value > 9 )
            {
                // the initial chartodval implementation allowed decimal marks even
                // in values that shouldn't have had a fractional part, so account
                // for that rare case here
                if( *text == _T('.') || *text == _T(',') )
                    return TextToDouble(text_end - text_length, text_length, 0);

                return DEFAULT;
            }

            value = value * 10 + digit_value;
        }

        ASSERT(text == text_end);

        return (double)( negative ? ( -1 * value ) : value );
    }


    // NUMBER -> TEXT
    template<typename T>
    inline bool IntegerDoubleToTextWorker(double non_negative_rounded_value, TCHAR* text, size_t text_length, bool zero_fill, bool negative)
    {
        ASSERT(non_negative_rounded_value >= 0 && !IsSpecial(non_negative_rounded_value));

        T integer_value = (T)non_negative_rounded_value;
        ASSERT((double)integer_value == non_negative_rounded_value);

        // construct the string from right to left
        TCHAR* rtl_text_iterator = text + text_length;

        do
        {
            if( --rtl_text_iterator < text )
                return false;

            *(rtl_text_iterator) = _T('0') + ( integer_value % 10 );
            integer_value /= 10;

        } while( integer_value != 0 );

        ASSERT(rtl_text_iterator >= text);

        if( rtl_text_iterator > text )
        {
            // add the negative sign
            if( negative )
            {
                // with zero fills, it comes at the beginning of the string;
                // otherwise it comes just before the number
                TCHAR* negative_sign_position = zero_fill ? (text++) : (--rtl_text_iterator);
                *negative_sign_position = _T('-');
            }

            // add zero fills or spaces
            TCHAR padding_character = zero_fill ? _T('0') : _T(' ');

            while( rtl_text_iterator > text )
                *(--rtl_text_iterator) = padding_character;
        }

        // if there is no space for the negative sign, then the value was too large
        else if( negative )
            return false;

        // a successful conversion
        return true;
    }


    template<typename T>
    inline void IntegerDoubleToText(double value, TCHAR* text, size_t text_length, bool zero_fill)
    {
        // format the string assuming that value can completely fit in the string;
        // if not it will be formatted as default
        if( !IsSpecial(value) )
        {
            bool negative = false;

            if( value < 0 )
            {
                negative = true;
                value = -1 * value;
            }

            // round the value
            value = floor(value + MAGICROUND);

            if( IntegerDoubleToTextWorker<T>(value, text, text_length, zero_fill, negative) )
                return;
        }

        // if still in the function, the value is a special value or too large for the buffer

        // notappl is a blank string, whereas anything else is default
        _tmemset(text, ( value == NOTAPPL ) ? _T(' ') : _T('*'), text_length);
    }


    inline void DoubleToText(double value, TCHAR* text, size_t text_length, size_t number_decimals, bool zero_fill, bool add_decimal_char)
    {
        // format the string assuming that value can completely fit in the string;
        // if not it will be formatted as default
        if( !IsSpecial(value) )
        {
            // don't use the more expensive double version unless necessary
            if( number_decimals == 0 )
            {
                IntegerDoubleToText<int64_t>(value, text, text_length, zero_fill);
                return;
            }

            bool negative = false;

            if( value < 0 )
            {
                negative = true;
                value = -1 * value;
            }

            // round the value at the correct decimal position
            value = value + MAGICROUND / Power10[number_decimals];

            // separate the integer and fractional parts
            double integer_part;
            double fractional_part = modf(value, &integer_part);

            // convert the integer part
            size_t actual_integer_length = text_length - number_decimals - ( add_decimal_char ? 1 : 0 );

            if( IntegerDoubleToTextWorker<int64_t>(integer_part, text, actual_integer_length, zero_fill, negative) )
            {
                TCHAR* fractional_text = text + actual_integer_length;

                // add the decimal mark if necessary
                if( add_decimal_char )
                    *(fractional_text++) = _T('.');

                // convert the fractional part, which will always be zero filled (as the zeros are to
                // the right of the decimal mark)
                fractional_part = floor(fractional_part *= Power10[number_decimals]);

                ASSERT(number_decimals <= 9);
                IntegerDoubleToTextWorker<int>(fractional_part, fractional_text, number_decimals, true, false);

                return;
            }
        }

        // if still in the function, the value is a special value or too large for the buffer

        // notappl is a blank string, whereas anything else is default
        _tmemset(text, ( value == NOTAPPL ) ? _T(' ') : _T('*'), text_length);
    }
}
