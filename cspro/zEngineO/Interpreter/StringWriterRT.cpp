#include "stdafx.h"
#include "IncludesRT.h"
#include "StringWriter.h"


Engine::Value LogicInterpreter::ex_StringWriter_clear(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    StringWriter& string_writer = GetSymbolStringWriter(symbol_va_node.symbol_index);

    std::string* text_builder;
    std::tie(std::ignore, text_builder) = GetTextTemplateBuilder(string_writer);

    if( text_builder == nullptr )
        return Engine::Value::Bool(false);

    text_builder->clear();

    return Engine::Value::Bool(true);
}


Engine::Value LogicInterpreter::ex_StringWriter_toString(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    StringWriter& string_writer = GetSymbolStringWriter(symbol_va_node.symbol_index);

    const std::string* text_builder;
    std::tie(std::ignore, text_builder) = GetTextTemplateBuilder(string_writer);

    if( text_builder == nullptr )
        return Engine::Value::Invalid<SharableString>();

    return *text_builder;
}
