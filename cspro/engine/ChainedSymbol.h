#pragma once

#include <zLogicO/Symbol.h>


// for symbols that need to be chained (for access operations)
class ChainedSymbol : public Symbol
{
public:
    int SYMTowner;              // Index of owner of this object in Symbol Table
    int SYMTfwd;                // Index of the next object of the same type
    ChainedSymbol* next_symbol;   // The next actual symbol of the same type

    ChainedSymbol(std::wstring name, SymbolType symbol_type)
        :   Symbol(std::move(name), symbol_type),
            SYMTowner(-1),
            SYMTfwd(-1),
            next_symbol(nullptr)
    {
    }
};
