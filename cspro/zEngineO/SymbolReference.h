#pragma once


// --------------------------------------------------------------------------
// evaluated subscript types
// --------------------------------------------------------------------------

struct EvaluatedEngineItemSubscript
{
    int record_occurrence = 0;
    int item_subitem_occurrence = 0;
};



// --------------------------------------------------------------------------
// SymbolReference
//
// The struct stores a reference to a particular symbol.
// --------------------------------------------------------------------------

template<typename T>
struct SymbolReference
{
    // a non-null pointer, or shared pointer, to the symbol
    T symbol;

    // the logic bytecode address of the subscript compilation
    int subscript_compilation;

    // the portions of the subscript evaluated from subscript compilation
    std::variant<std::monostate, EvaluatedEngineItemSubscript> evaluated_subscript;
};
