#pragma once

#include <engine/Defines.h>
#include <zLogicO/Token.h>


namespace EngineStringComparer
{
    constexpr TokenCode NumericFunctionCodeToTokenCode(FunctionCode function_code);
    constexpr TokenCode StringFunctionCodeToTokenCode(FunctionCode function_code);

    namespace V0
    {
        int Compare(const TCHAR* lhs_itr, const TCHAR* rhs_itr);
        int Compare(const std::wstring& lhs_text, const std::wstring& rhs_text);
        bool Evaluate(const std::wstring& lhs_text, const std::wstring& rhs_text, TokenCode token_code);
    }

    namespace V8
    {
        template<TokenCode token_code>
        bool Evaluate(const std::wstring& lhs, const std::wstring& rhs);

        bool Evaluate(const std::wstring& lhs, const std::wstring& rhs, TokenCode token_code);
    }
}


inline constexpr TokenCode EngineStringComparer::NumericFunctionCodeToTokenCode(FunctionCode function_code)
{
    static_assert(static_cast<TokenCode>(FunctionCode::EQ_CODE) == TokenCode::TOKEQOP);
    static_assert(static_cast<TokenCode>(FunctionCode::NE_CODE) == TokenCode::TOKNEOP);
    static_assert(static_cast<TokenCode>(FunctionCode::LT_CODE) == TokenCode::TOKLTOP);
    static_assert(static_cast<TokenCode>(FunctionCode::LE_CODE) == TokenCode::TOKLEOP);
    static_assert(static_cast<TokenCode>(FunctionCode::GE_CODE) == TokenCode::TOKGEOP);
    static_assert(static_cast<TokenCode>(FunctionCode::GT_CODE) == TokenCode::TOKGTOP);

    ASSERT(function_code >= FunctionCode::EQ_CODE && function_code <= FunctionCode::GT_CODE);
    return static_cast<TokenCode>(static_cast<int>(function_code));
}


inline constexpr TokenCode EngineStringComparer::StringFunctionCodeToTokenCode(FunctionCode function_code)
{
    constexpr int StringNumericOperatorDifference = FunctionCode::CH_EQ_CODE - FunctionCode::EQ_CODE;
    static_assert(StringNumericOperatorDifference == ( FunctionCode::CH_NE_CODE - FunctionCode::NE_CODE ));
    static_assert(StringNumericOperatorDifference == ( FunctionCode::CH_LT_CODE - FunctionCode::LT_CODE ));
    static_assert(StringNumericOperatorDifference == ( FunctionCode::CH_LE_CODE - FunctionCode::LE_CODE ));
    static_assert(StringNumericOperatorDifference == ( FunctionCode::CH_GE_CODE - FunctionCode::GE_CODE ));
    static_assert(StringNumericOperatorDifference == ( FunctionCode::CH_GT_CODE - FunctionCode::GT_CODE ));

    ASSERT(function_code >= FunctionCode::CH_EQ_CODE && function_code <= FunctionCode::CH_GT_CODE);
    return NumericFunctionCodeToTokenCode(static_cast<FunctionCode>(static_cast<int>(function_code) - StringNumericOperatorDifference));
}


inline int EngineStringComparer::V0::Compare(const TCHAR* lhs_itr, const TCHAR* rhs_itr)
{
    // if one of the strings is longer than the other, assume that
    // the shorter string has blanks to match the length of the longer string
    while( true )
    {
        int ch_diff;

        // get the lhs character
        if( *lhs_itr == 0 )
        {
            if( *rhs_itr == 0 )
                return 0;

            ch_diff = BLANK;
        }

        else
        {
            ch_diff = *(lhs_itr++);
        }

        // subtract the rhs character
        ch_diff -= ( *rhs_itr == 0 ) ? BLANK :
                                       *(rhs_itr++);

        if( ch_diff != 0 )
        {
            return ch_diff > 0 ?  1 :
                                 -1;
        }
    }
}


inline int EngineStringComparer::V0::Compare(const std::wstring& lhs_text, const std::wstring& rhs_text)
{
    return Compare(lhs_text.c_str(), rhs_text.c_str());
}


inline bool EngineStringComparer::V0::Evaluate(const std::wstring& lhs_text, const std::wstring& rhs_text, TokenCode token_code)
{
    const int comparison = Compare(lhs_text, rhs_text);
    ASSERT80(comparison == 0 || comparison == -1 || comparison == 1);

    if( token_code == TokenCode::TOKEQOP )
    {
        return ( comparison == 0 );
    }

    else if( token_code == TokenCode::TOKNEOP )
    {
        return ( comparison != 0 );
    }

    else if( token_code == TokenCode::TOKLTOP )
    {
        return ( comparison == -1 );
    }

    else if( token_code == TokenCode::TOKLEOP )
    {
        return ( comparison <= 0 );
    }

    else if( token_code == TokenCode::TOKGEOP )
    {
        return ( comparison >= 0 );
    }

    else if( token_code == TokenCode::TOKGTOP )
    {
        return ( comparison == 1 );
    }

    else
    {
        return ReturnProgrammingError(0);
    }
}


template<TokenCode token_code>
bool EngineStringComparer::V8::Evaluate(const std::wstring& lhs, const std::wstring& rhs)
{
    if constexpr(token_code == TokenCode::TOKEQOP)
    {
        return ( lhs == rhs );
    }

    else if constexpr(token_code == TokenCode::TOKNEOP)
    {
        return ( lhs != rhs );
    }

    else
    {
        const int compare = lhs.compare(rhs);

        if constexpr(token_code == TokenCode::TOKLTOP)
        {
            return ( compare < 0 );
        }

        else if constexpr(token_code == TokenCode::TOKLEOP)
        {
            return ( compare <= 0 );
        }

        else if constexpr(token_code == TokenCode::TOKGEOP)
        {
            return ( compare >= 0 );
        }

        else if constexpr(token_code == TokenCode::TOKGTOP)
        {
            return ( compare > 0 );
        }

        else
        {
            static_assert_false();
        }
    }
}


inline bool EngineStringComparer::V8::Evaluate(const std::wstring& lhs, const std::wstring& rhs, TokenCode token_code)
{
    return ( token_code == TokenCode::TOKEQOP ) ? Evaluate<TokenCode::TOKEQOP>(lhs, rhs) :
           ( token_code == TokenCode::TOKNEOP ) ? Evaluate<TokenCode::TOKNEOP>(lhs, rhs) :
           ( token_code == TokenCode::TOKLTOP ) ? Evaluate<TokenCode::TOKLTOP>(lhs, rhs) :
           ( token_code == TokenCode::TOKLEOP ) ? Evaluate<TokenCode::TOKLEOP>(lhs, rhs) :
           ( token_code == TokenCode::TOKGEOP ) ? Evaluate<TokenCode::TOKGEOP>(lhs, rhs) :
           ( token_code == TokenCode::TOKGTOP ) ? Evaluate<TokenCode::TOKGTOP>(lhs, rhs) :
                                                  ReturnProgrammingError(false);
}
