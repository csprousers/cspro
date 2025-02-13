#pragma once

#include <zToolsO/FloatingPointMath.h>
#include <zLogicO/Token.h>


namespace EngineFloatingPointMath
{
    template<TokenCode token_code>
    bool comparison(double lhs, double rhs)
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
