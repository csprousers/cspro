#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineItem.h"
#include "ValueSet.h"
#include <engine/VarT.h>
#include <zDictO/ValueProcessor.h>
#include <zDictO/ValueSetResponse.h>


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



// --------------------------------------------------------------------------
// Item functions
// --------------------------------------------------------------------------

SharableString LogicInterpreter::GetItemValueLabel(const VART& vart, const std::variant<double, SharableString>& value)
{
    ASSERT(vart.IsAlpha() == std::holds_alternative<SharableString>(value));

    // three passes to evaluate the label:
    // 1) look at the current value set
    // 2) look at the base value set
    const ValueSet* value_set = vart.GetCurrentValueSet();

    while( value_set != nullptr )
    {
        const ValueProcessor& value_processor = value_set->GetValueProcessor();

        const DictValue* const dict_value = vart.IsAlpha()
            ? value_processor.GetDictValue(*std::get<SharableString>(value))
            : value_processor.GetDictValue(std::get<double>(value));

        if( dict_value != nullptr )
            return UTF8_TODO::GetUtf8(dict_value->GetLabel());

        // if not in the current value set, check the base value set
        const ValueSet* const base_value_set = vart.GetBaseValueSet();

        if( value_set == base_value_set )
            break;

        value_set = base_value_set;
    }

    // 3) format the code nicely
    if( vart.IsAlpha() )
    {
        return SharableString(std::get<SharableString>(value)).MakeTrim();
    }

    else
    {
        return ValueSetResponse::FormatValueForDisplay(*vart.GetDictItem(), std::get<double>(value));
    }
}


Engine::Value LogicInterpreter::ex_getvaluelabel(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const VART& vart = GetSymbol<VART>(va_node.arguments[0]);
    const std::variant<double, SharableString> value = EvaluateVariant<SharableString>(vart.GetDataType(), va_node.arguments[1]);

    return GetItemValueLabel(vart, value);
}
