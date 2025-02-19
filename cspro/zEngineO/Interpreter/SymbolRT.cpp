#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineItem.h"


double LogicInterpreter::ex_Symbol_getLabel(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    const SharableString language = EvaluateNullableSharableString(symbol_va_with_subscript_node.arguments[0]);
    const Symbol& symbol = GetFromSymbolOrEngineItemForStaticFunction(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    // if no language is defined, or it is blank, we can return the label directly
    if( !language.IsSet() || language->empty() )
        return AssignString(SymbolCalculator::GetLabel(symbol));

    // otherwise we have to look for the label in the dictionary, which is currently the only location where labels can have multiple languages
    const EngineItemAccessor* const engine_item_accessor = symbol.GetEngineItemAccessor();

    if( engine_item_accessor != nullptr )
    {
        const CDictItem& dict_item = engine_item_accessor->GetDictItem();
        const std::optional<size_t> language_index = dict_item.GetRecord()->GetDataDict()->IsLanguageDefined(*language);

        if( language_index.has_value() )
            return AssignString(UTF8_TODO::GetUtf8(dict_item.GetLabelSet().GetLabel(*language_index, false)));
    }

    return AssignStringNull();
}


double LogicInterpreter::ex_Symbol_getName(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    const Symbol& symbol = GetFromSymbolOrEngineItemForStaticFunction(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    return AssignString(symbol.GetName());
}
