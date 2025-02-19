#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineItem.h"


// --------------------------------------------------------------------------
// Item node handling and occurrence calculators
// --------------------------------------------------------------------------

const Nodes::SymbolVariableArgumentsWithSubscript& LogicInterpreter::GetOrConvertPre80SymbolVariableArgumentsWithSubscriptNode(const int program_index)
{
    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        return GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);

    // check the cache of already converted nodes
    const auto& lookup = m_convertedPre80Nodes.find(program_index);

    if( lookup != m_convertedPre80Nodes.cend() )
        return *reinterpret_cast<const Nodes::SymbolVariableArgumentsWithSubscript*>(lookup->second.get());

    // convert a SymbolVariableArguments node to a SymbolVariableArgumentsWithSubscript node
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);

    // we don't know how many arguments were specified, so assume up to 1000
    const size_t arguments = std::min<size_t>(1000, m_engineData->logic_byte_code.GetSize() - program_index);

    auto byte_code = std::make_unique_for_overwrite<int[]>(( sizeof(Nodes::SymbolVariableArgumentsWithSubscript) / sizeof(int) ) + arguments);
    auto& symbol_va_with_subscript_node = *reinterpret_cast<Nodes::SymbolVariableArgumentsWithSubscript*>(byte_code.get());

    symbol_va_with_subscript_node.function_code = symbol_va_node.function_code;

    symbol_va_with_subscript_node.symbol_index = symbol_va_node.symbol_index;
    ASSERT(NPT_Ref(symbol_va_with_subscript_node.symbol_index).IsOneOf(SymbolType::Audio, SymbolType::Document, SymbolType::Image));

    symbol_va_with_subscript_node.subscript_compilation = -1;

    memcpy(symbol_va_with_subscript_node.arguments, symbol_va_node.arguments, sizeof(int) * arguments);

    m_convertedPre80Nodes.try_emplace(program_index, std::move(byte_code));

    return symbol_va_with_subscript_node;
}


const Nodes::SymbolComputeWithSubscript& LogicInterpreter::GetOrConvertPre80SymbolComputeWithSubscriptNode(const int program_index)
{
    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        return GetNode<Nodes::SymbolComputeWithSubscript>(program_index);

    // check the cache of already converted nodes
    const auto& lookup = m_convertedPre80Nodes.find(program_index);

    if( lookup != m_convertedPre80Nodes.cend() )
        return *reinterpret_cast<const Nodes::SymbolComputeWithSubscript*>(lookup->second.get());

    // convert a SymbolCompute node to a SymbolComputeWithSubscript node
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(program_index);

    auto byte_code = std::make_unique_for_overwrite<int[]>(sizeof(Nodes::SymbolComputeWithSubscript) / sizeof(int));
    auto& symbol_compute_with_subscript_node = *reinterpret_cast<Nodes::SymbolComputeWithSubscript*>(byte_code.get());

    symbol_compute_with_subscript_node.function_code = symbol_compute_node.function_code;
    symbol_compute_with_subscript_node.next_st = symbol_compute_node.next_st;
    symbol_compute_with_subscript_node.lhs_symbol_index = symbol_compute_node.lhs_symbol_index;
    symbol_compute_with_subscript_node.lhs_subscript_compilation = -1;
    symbol_compute_with_subscript_node.rhs_symbol_index = symbol_compute_node.rhs_symbol_index;
    symbol_compute_with_subscript_node.rhs_subscript_compilation = -1;

    m_convertedPre80Nodes.try_emplace(program_index, std::move(byte_code));

    return symbol_compute_with_subscript_node;
}


template<typename SymbolT>
SymbolT LogicInterpreter::EvaluateSymbolReference_GetSymbol(const int symbol_index)
{
    if constexpr(std::is_same_v<SymbolT, Symbol*>)
    {
        return &m_symbolTable.GetAt(symbol_index);
    }

    else
    {
        return m_symbolTable.GetSharedAt(symbol_index);
    }
}


template<typename T/* = Symbol* */>
SymbolReference<T> LogicInterpreter::EvaluateSymbolReference(int symbol_index, int subscript_compilation)
{
    ASSERT(symbol_index != -1);

    SymbolReference<T> symbol_reference
    {
        EvaluateSymbolReference_GetSymbol<T>(symbol_index),
        subscript_compilation,
        std::monostate()
    };

    // if an item, evaluate the subscript
    if( symbol_reference.symbol->IsA(SymbolType::Item) )
    {
        ASSERT(subscript_compilation != -1);
        const auto& item_subscript_node = GetNode<Nodes::ItemSubscript>(subscript_compilation);

        symbol_reference.evaluated_subscript = EvaluateEngineItemSubscript(assert_cast<const EngineItem&>(*symbol_reference.symbol), item_subscript_node);
    }

    // if not an item, there is nothing to evaluate
    else
    {
        ASSERT(subscript_compilation == -1);
    }

    return symbol_reference;
}

// INTERPRETER_DLL_TODO remove ZENGINEO_API
template ZENGINEO_API SymbolReference<Symbol*> LogicInterpreter::EvaluateSymbolReference(int symbol_index, int subscript_compilation);
template ZENGINEO_API SymbolReference<std::shared_ptr<Symbol>> LogicInterpreter::EvaluateSymbolReference(int symbol_index, int subscript_compilation);
