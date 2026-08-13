#pragma once

#include <zLogicO/SymbolType.h>
#include <zLogicO/TokenCode.h>
#include <engine/VarT.h>


constexpr bool IsArithmeticOperator(TokenCode token_code);
constexpr bool IsRelationalOperator(TokenCode token_code);

// Determines if a token is a real VART object and not a WorkVariable (since they both use TOKVAR as their token code).
// Returns true if Tkn == TOKVAR and Symbol::IsA(SymbolType::Variable).
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
    if( compiler.GetCurrentToken().code == TOKVAR )
    {
        const Symbol& symbol = compiler.GetSymbolTable().GetAt(const_cast<T&>(compiler).get_COMPILER_DLL_TODO_Tokstindex());

        if( symbol.IsA(SymbolType::Variable) )
        {
            // if the assert is always true, get rid of places that check if VART::GetDictItem is null
            // and remove the inclusion of engine/VarT.h above
            ASSERT82(assert_cast<const VART&>(symbol).GetDictItem() != nullptr);
            return true;
        }
    }

    return false;
}
