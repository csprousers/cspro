#pragma once

#include <zToolsO/Special.h>
#include <zLogicO/Token.h>


class FloatingPointMath
{
    /*--------------------------------------------------------------------------
        Relational operators with "fuzzy noise" due to floating point rounding
        gsf 03-jun-2004

        Basic problem is that two floating point numbers may really be "equal"
        from user's point of view, but due to rounding inherent in floating point
        arithmetic, they really differ by a small amount.

        Solution is to test the difference and see if it is smaller than some
        threshold amount.  The threshold amount is very small, but larger than
        the "noise".  Following this line of thinking, if the threshold amount is T:

        a = b       fabs(a - b) <= T
        a != b      fabs(a - b) > T
        a > b       a - b > T
        a >= b      a - b > -T
        a < b       a - b < -T
        a <= b      a - b < T

        The threshold level depends on the magnitude of the numbers compared.  My
        empirical testing shows that the "noise" shows up 16 decimal digits away from
        the most significant decimal digit.  This is why I divide by 10 to the 14th
        power to determine the threshold.
    ----------------------------------------------------------------------------*/

public:
    enum class Operation { Equals, LessThan, LessThanEquals, GreaterThanEquals, GreaterThan };

    static constexpr double FloatingPointThreshold = 10E-13;

    // handles: = < <= >= >
    template<Operation operation>
    static bool Evaluate(double lhs, double rhs);

    static bool Equals(double lhs, double rhs)            { return Evaluate<Operation::Equals>(lhs, rhs); }
    static bool LessThan(double lhs, double rhs)          { return Evaluate<Operation::LessThan>(lhs, rhs); }
    static bool LessThanEquals(double lhs, double rhs)    { return Evaluate<Operation::LessThanEquals>(lhs, rhs); }
    static bool GreaterThanEquals(double lhs, double rhs) { return Evaluate<Operation::GreaterThanEquals>(lhs, rhs); }
    static bool GreaterThan(double lhs, double rhs)       { return Evaluate<Operation::GreaterThan>(lhs, rhs); }

    // handles the above, along with <>, with processing for special values
    template<TokenCode token_code>
    static bool Evaluate(double lhs, double rhs);

    static bool Evaluate(TokenCode token_code, double lhs, double rhs);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<FloatingPointMath::Operation operation>
bool FloatingPointMath::Evaluate(const double lhs, const double rhs)
{
    if constexpr(operation == Operation::Equals)
    {
        // short circuit a likely result
        if( lhs == rhs )
            return true;
    }

    else
    {
        // special value processing should be handled elsewhere
        ASSERT(!IsSpecial(lhs) && !IsSpecial(rhs));
    }

    const double value_difference = lhs - rhs;
    const double threshold = std::max(fabs(lhs / 100000000000000), FloatingPointThreshold);

    if constexpr(operation == Operation::Equals)
    {
        return ( fabs(value_difference) <= threshold );
    }

    else if constexpr(operation == Operation::LessThan)
    {
        return ( value_difference < -threshold );
    }

    else if constexpr(operation == Operation::LessThanEquals)
    {
        return ( value_difference <= threshold );
    }

    else if constexpr(operation == Operation::GreaterThanEquals)
    {
        return ( value_difference >= -threshold );
    }

    else if constexpr(operation == Operation::GreaterThan)
    {
        return ( value_difference > threshold );
    }

    else
    {
        static_assert_false();
    }
}


template<TokenCode token_code>
bool FloatingPointMath::Evaluate(const double lhs, const double rhs)
{
    if constexpr(token_code == TokenCode::TOKEQOP)
    {
        return FloatingPointMath::Equals(lhs, rhs);
    }

    else if constexpr(token_code == TokenCode::TOKNEOP)
    {
        return !FloatingPointMath::Equals(lhs, rhs);
    }

    else
    {
        if( IsSpecial(lhs) || IsSpecial(rhs) )
            return false;

        if constexpr(token_code == TokenCode::TOKLTOP)
        {
            return FloatingPointMath::LessThan(lhs, rhs);
        }

        else if constexpr(token_code == TokenCode::TOKLEOP)
        {
            return FloatingPointMath::LessThanEquals(lhs, rhs);
        }

        else if constexpr(token_code == TokenCode::TOKGEOP)
        {
            return FloatingPointMath::GreaterThanEquals(lhs, rhs);
        }

        else if constexpr(token_code == TokenCode::TOKGTOP)
        {
            return FloatingPointMath::GreaterThan(lhs, rhs);
        }

        else
        {
            static_assert_false();
        }
    }
}


inline bool FloatingPointMath::Evaluate(const TokenCode token_code, const double lhs, const double rhs)
{
    switch( token_code )
    {
        case TokenCode::TOKEQOP: return Evaluate<TokenCode::TOKEQOP>(lhs, rhs);
        case TokenCode::TOKNEOP: return Evaluate<TokenCode::TOKNEOP>(lhs, rhs);
        case TokenCode::TOKLTOP: return Evaluate<TokenCode::TOKLTOP>(lhs, rhs);
        case TokenCode::TOKLEOP: return Evaluate<TokenCode::TOKLEOP>(lhs, rhs);
        case TokenCode::TOKGEOP: return Evaluate<TokenCode::TOKGEOP>(lhs, rhs);
        case TokenCode::TOKGTOP: return Evaluate<TokenCode::TOKGTOP>(lhs, rhs);
        default:                 return ReturnProgrammingError(false);
    }
}
