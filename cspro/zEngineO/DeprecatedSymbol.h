#pragma once

#include <zLogicO/Symbol.h>


class DeprecatedSymbol : public Symbol
{
public:
    DeprecatedSymbol(std::wstring name, SymbolType symbol_type)
        :   Symbol(std::move(name), symbol_type)
    {
        ASSERT(symbol_type == SymbolType::Index_Unused);
    }
};
