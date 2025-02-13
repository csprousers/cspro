#pragma once

#include <zLogicO/SymbolType.h>
#include <zLogicO/TokenCode.h>


constexpr bool IsArithmeticOperator(TokenCode token_code);
constexpr bool IsRelationalOperator(TokenCode token_code);

// determines if a token is a real VART object and not a WorkVariable (since they both use TOKVAR as their token code);
// returns true if Tkn == TOKVAR and Symbol::IsA(SymbolType::Variable)
template<typename T>
bool IsCurrentTokenVART(const T& compiler);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr bool IsArithmeticOperator(TokenCode token_code)
{
    switch( token_code )
    {
        case TokenCode::TOKADDOP:
        case TokenCode::TOKMINOP:
        case TokenCode::TOKMULOP:
        case TokenCode::TOKDIVOP:
        case TokenCode::TOKMODOP:
        case TokenCode::TOKEXPOP:
        case TokenCode::TOKMINUS:
            return true;

        default:
            return false;
    }
}


constexpr bool IsRelationalOperator(TokenCode token_code)
{
    switch( token_code )
    {
        case TokenCode::TOKEQOP:
        case TokenCode::TOKNEOP:
        case TokenCode::TOKLTOP:
        case TokenCode::TOKLEOP:
        case TokenCode::TOKGTOP:
        case TokenCode::TOKGEOP:
            return true;

        default:
            return false;
    }
}


template<typename T>
bool IsCurrentTokenVART(const T& compiler)
{
    return ( compiler.GetCurrentToken().code == TOKVAR &&
             compiler.GetSymbolTable().GetAt(const_cast<T&>(compiler).get_COMPILER_DLL_TODO_Tokstindex()).IsA(SymbolType::Variable) );
}
