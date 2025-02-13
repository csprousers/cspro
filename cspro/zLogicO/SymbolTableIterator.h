#pragma once

#include <zLogicO/SymbolTable.h>
#include <zEngineO/AllSymbolDeclarations.h>


template<typename ST>
constexpr SymbolType ToSymbolType()
{
         if constexpr(std::is_same_v<ST, RELT>)         return SymbolType::Relation;
    else if constexpr(std::is_same_v<ST, UserFunction>) return SymbolType::UserFunction;
    else                                                static_assert_false();
}


template<typename ST, bool CallbackReturnsTrueToContinue/* = false*/, typename CF>
auto Logic::SymbolTable::ForeachSymbol(CF callback_function)
{
    SymbolType symbol_type = ToSymbolType<ST>();

    ASSERT(!m_symbols.empty() && m_symbols.front() == nullptr);

    auto symbols_itr = m_symbols.begin() + FirstValidSymbolIndex;
    auto symbols_end = m_symbols.end();

    ASSERT(symbols_itr <= symbols_end);

    for( ; symbols_itr != symbols_end; ++symbols_itr )
    {
        Symbol& symbol = *(*symbols_itr);

        if( !symbol.IsA(symbol_type) )
            continue;

        if constexpr(CallbackReturnsTrueToContinue)
        {
            if( !callback_function(assert_cast<ST&>(symbol)) )
                return symbol.GetSymbolIndex();
        }

        else
        {
            callback_function(assert_cast<ST&>(symbol));
        }
    }

    if constexpr(CallbackReturnsTrueToContinue)
        return -1;
}
