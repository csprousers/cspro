#pragma once

#include <zToolsO/Special.h>


namespace FloatingPointMath
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

    enum class Operation { Equals, LessThan, LessThanEquals, GreaterThanEquals, GreaterThan };

    template<Operation operation>
    static bool Evaluate(double lhs, double rhs)
    {
        constexpr double FloatingPointThreshold = 10E-13;

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

        double value_difference = lhs - rhs;
        double threshold = std::max(fabs(lhs / 100000000000000), FloatingPointThreshold);

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

    inline bool Equals(double lhs, double rhs)            { return Evaluate<Operation::Equals>(lhs, rhs); }
    inline bool LessThan(double lhs, double rhs)          { return Evaluate<Operation::LessThan>(lhs, rhs); }
    inline bool LessThanEquals(double lhs, double rhs)    { return Evaluate<Operation::LessThanEquals>(lhs, rhs); }
    inline bool GreaterThanEquals(double lhs, double rhs) { return Evaluate<Operation::GreaterThanEquals>(lhs, rhs); }
    inline bool GreaterThan(double lhs, double rhs)       { return Evaluate<Operation::GreaterThan>(lhs, rhs); }
};
