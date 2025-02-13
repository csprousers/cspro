#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/Strings.h"
#include <zLogicO/LocalSymbolStack.h>


int LogicCompiler::CreateNumericConstantNode(const double numeric_constant)
{
    auto& const_node = CreateNode<CONST_NODE>(FunctionCode::CONST_CODE);

    const_node.const_index = ConserveConstant(numeric_constant);

    return GetProgramIndex(const_node);
}


int LogicCompiler::CreateStringLiteralNode(std::wstring string_literal)
{
    auto& string_literal_node = CreateNode<Nodes::StringLiteral>(FunctionCode::STRING_LITERAL_CODE);

    string_literal_node.string_literal_index = ConserveConstant(std::move(string_literal));

    return GetProgramIndex(string_literal_node);
}


int LogicCompiler::CreateOperatorNode(const FunctionCode function_code, const int left_expr, const int right_expr)
{
    auto& operator_node = CreateNode<Nodes::Operator>(function_code);

    operator_node.left_expr = left_expr;
    operator_node.right_expr = right_expr;

    return GetProgramIndex(operator_node);
}


int LogicCompiler::CreateListNode(const cs::span<const int> arguments)
{
    auto& list_node = CreateVariableSizeNode<Nodes::List>(arguments.size());

    list_node.number_elements = static_cast<int>(arguments.size());

    if( !arguments.empty() )
        memcpy(list_node.elements, arguments.data(), arguments.size() * sizeof(arguments[0]));

    return GetProgramIndex(list_node);
}


int LogicCompiler::CreateVariableArgumentsNode(const FunctionCode function_code, const cs::span<const int> arguments)
{
    auto& va_node = CreateVariableSizeNode<Nodes::VariableArguments>(function_code, arguments.size());

    if( !arguments.empty() )
        memcpy(va_node.arguments, arguments.data(), arguments.size() * sizeof(arguments[0]));

    return GetProgramIndex(va_node);
}


int LogicCompiler::CreateVariableArgumentsWithSizeNode(const FunctionCode function_code, const cs::span<const int> arguments)
{
    auto& va_with_size_node = CreateVariableSizeNode<Nodes::VariableArgumentsWithSize>(function_code, arguments.size());

    va_with_size_node.number_arguments = static_cast<int>(arguments.size());

    if( !arguments.empty() )
        memcpy(va_with_size_node.arguments, arguments.data(), arguments.size() * sizeof(arguments[0]));

    return GetProgramIndex(va_with_size_node);
}


int LogicCompiler::CreateSymbolVariableArgumentsNode(const FunctionCode function_code, const Symbol& symbol, const cs::span<const int> arguments)
{
    auto& symbol_va_node = CreateNode<Nodes::SymbolVariableArguments>(function_code, arguments.size() - 1);

    symbol_va_node.symbol_index = symbol.GetSymbolIndex();

    if( !arguments.empty() )
        memcpy(symbol_va_node.arguments, arguments.data(), arguments.size() * sizeof(arguments[0]));

    return GetProgramIndex(symbol_va_node);
}


Nodes::SymbolVariableArguments& LogicCompiler::CreateSymbolVariableArgumentsNode(const FunctionCode function_code, const Symbol& symbol,
                                                                                 const int number_arguments, const std::optional<int> initialize_value/* = std::nullopt*/)
{
    auto& symbol_va_node = CreateNode<Nodes::SymbolVariableArguments>(function_code, number_arguments - 1);

    symbol_va_node.symbol_index = symbol.GetSymbolIndex();

    if( initialize_value.has_value() )
    {
        for( int i = 0; i < number_arguments; ++i )
            symbol_va_node.arguments[i] = *initialize_value;
    }

    return symbol_va_node;
}


int LogicCompiler::CreateSymbolVariableArgumentsWithSubscriptNode(const FunctionCode function_code, const Symbol& symbol,
                                                                  const int symbol_subscript_compilation, const cs::span<const int> arguments)
{
    auto& symbol_va_with_subscript_node = CreateNode<Nodes::SymbolVariableArgumentsWithSubscript>(function_code, arguments.size() - 1);

    symbol_va_with_subscript_node.symbol_index = symbol.GetSymbolIndex();
    symbol_va_with_subscript_node.subscript_compilation = symbol_subscript_compilation;

    if( !arguments.empty() )
        memcpy(symbol_va_with_subscript_node.arguments, arguments.data(), arguments.size() * sizeof(arguments[0]));

    return GetProgramIndex(symbol_va_with_subscript_node);
}


Nodes::SymbolVariableArgumentsWithSubscript& LogicCompiler::CreateSymbolVariableArgumentsWithSubscriptNode(const FunctionCode function_code, const Symbol& symbol,
                                                                                                           const int symbol_subscript_compilation,
                                                                                                           const int number_arguments, const std::optional<int> initialize_value/* = std::nullopt*/)
{
    auto& symbol_va_with_subscript_node = CreateNode<Nodes::SymbolVariableArgumentsWithSubscript>(function_code, number_arguments - 1);

    symbol_va_with_subscript_node.symbol_index = symbol.GetSymbolIndex();
    symbol_va_with_subscript_node.subscript_compilation = symbol_subscript_compilation;

    if( initialize_value.has_value() )
    {
        for( int i = 0; i < number_arguments; ++i )
            symbol_va_with_subscript_node.arguments[i] = *initialize_value;
    }

    return symbol_va_with_subscript_node;
}


int LogicCompiler::WrapNodeAroundScopeChange(const Logic::LocalSymbolStack& local_symbol_stack, const int program_index,
                                             const bool store_local_symbol_names/* = false*/)
{
    if( program_index == -1 || local_symbol_stack.GetLocalSymbolIndices().empty() )
        return program_index;

    auto& scope_change_node = CreateNode<Nodes::ScopeChange>(FunctionCode::SCOPE_CHANGE_CODE);

    scope_change_node.next_st = -1;
    scope_change_node.program_index = program_index;
    scope_change_node.local_symbol_indices_list = CreateListNode(local_symbol_stack.GetLocalSymbolIndices());

    if( store_local_symbol_names )
    {
        auto symbol_names = std::make_unique_for_overwrite<int[]>(local_symbol_stack.GetLocalSymbolIndices().size());
        int* symbol_names_itr = symbol_names.get();

        for( const int symbol_index : local_symbol_stack.GetLocalSymbolIndices() )
        {
            *symbol_names_itr = ConserveConstant(NPT_Ref(symbol_index).GetName());
            ++symbol_names_itr;
        }

        scope_change_node.local_symbol_names_list = CreateListNode(cs::span<const int>(symbol_names.get(), local_symbol_stack.GetLocalSymbolIndices().size()));
    }

    else
    {
        scope_change_node.local_symbol_names_list = -1;
    }

    return GetProgramIndex(scope_change_node);
}
