#include "stdafx.h"
#include "IncludesRT.h"
#include "HashMap.h"
#include "List.h"
#include "SubscriptText.h"


std::vector<LogicHashMap::Data> LogicInterpreter::EvaluateHashMapIndex(const Nodes::List& dimension_expressions_node,
                                                                       const int number_dimension_expressions)
{
    std::vector<LogicHashMap::Data> dimension_values;

    for( int i = 0; i < number_dimension_expressions; i += 2 )
    {
        dimension_values.emplace_back(EvaluateVariant(static_cast<DataType>(dimension_expressions_node.elements[i]),
                                                      dimension_expressions_node.elements[i + 1]));
    }

    return dimension_values;
}


std::vector<LogicHashMap::Data> LogicInterpreter::EvaluateHashMapIndex(const Nodes::List& dimension_expressions_node)
{
    return EvaluateHashMapIndex(dimension_expressions_node, dimension_expressions_node.number_elements);
}


std::vector<LogicHashMap::Data> LogicInterpreter::EvaluateHashMapIndex(const int hashmap_node_expression, LogicHashMap** const out_hashmap, const bool bounds_checking)
{
    const auto& element_reference_node = GetNode<Nodes::ElementReference>(hashmap_node_expression);
    LogicHashMap*& hashmap = *out_hashmap;
    ASSERT(element_reference_node.function_code == HASHMAP_VAR_CODE);

    hashmap = &GetSymbolLogicHashMap(element_reference_node.symbol_index);

    std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(GetListNode(element_reference_node.element_expressions[0]));
    ASSERT(dimension_values.size() == hashmap->GetNumberDimensions());

    if( bounds_checking && !hashmap->HasDefaultValue() && !hashmap->Contains(dimension_values) )
    {
        IssueMessage(MessageType::Error, MGF::invalid_subscript_1008, hashmap->GetName().c_str(), GetSubscriptText(dimension_values).c_str());
        dimension_values.clear();
    }

    return dimension_values;
}


double LogicInterpreter::ex_HashMap_var(const int program_index)
{
    const LogicHashMap* hashmap;
    std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(program_index, const_cast<LogicHashMap**>(&hashmap), true);

    if( dimension_values.empty() )
        return AssignInvalidValue(hashmap->GetValueType());

    std::optional<LogicHashMap::Data> value = hashmap->GetValue(dimension_values);
    ASSERT(value.has_value());

    return AssignVariantValue(std::move(*value));
}


double LogicInterpreter::ex_HashMap_compute(const int program_index)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(program_index);

    // assigning to an element
    if( symbol_compute_node.rhs_symbol_type == SymbolType::None )
    {
        LogicHashMap* hashmap;
        const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_compute_node.lhs_symbol_index, &hashmap, false);

        if( hashmap->IsValueTypeNumeric() )
        {
            double value = Evaluate(symbol_compute_node.rhs_symbol_index);
            hashmap->SetValue(dimension_values, value);
            return value;
        }

        else
        {
            hashmap->SetValue(dimension_values, EvaluateSharableString(symbol_compute_node.rhs_symbol_index));
        }
    }

    // assigning a hashmap to a hashmap
    else
    {
        ASSERT(symbol_compute_node.rhs_symbol_type == SymbolType::HashMap);

        LogicHashMap& lhs_hashmap = GetSymbolLogicHashMap(symbol_compute_node.lhs_symbol_index);
        const LogicHashMap& rhs_hashmap = GetSymbolLogicHashMap(symbol_compute_node.rhs_symbol_index);

        lhs_hashmap = rhs_hashmap;
    }

    return 0;
}


double LogicInterpreter::ex_HashMap_clear(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    hashmap.Reset();

    return 1;
}


double LogicInterpreter::ex_HashMap_contains(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(GetListNode(symbol_va_node.arguments[0]));
    ASSERT(!dimension_values.empty() && dimension_values.size() <= hashmap.GetNumberDimensions());

    return hashmap.Contains(dimension_values);
}


double LogicInterpreter::ex_HashMap_length(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(GetListNode(symbol_va_node.arguments[0]));
    ASSERT(dimension_values.size() < hashmap.GetNumberDimensions());

    return hashmap.GetLength(dimension_values);
}


double LogicInterpreter::ex_HashMap_remove(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(GetListNode(symbol_va_node.arguments[0]));
    ASSERT(!dimension_values.empty() && dimension_values.size() <= hashmap.GetNumberDimensions());

    return hashmap.Remove(dimension_values) ? 1 : 0;
}


double LogicInterpreter::ex_HashMap_getKeys(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    const LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    const Nodes::List& arguments_node = GetListNode(symbol_va_node.arguments[0]);
    ASSERT(arguments_node.number_elements > 0);

    LogicList& getkeys_list = GetSymbolLogicList(arguments_node.elements[arguments_node.number_elements - 1]);

    if( getkeys_list.IsReadOnly() )
    {
        IssueMessage(MessageType::Error, MGF::List_read_only_cannot_be_modified_965, getkeys_list.GetName().c_str());
        return DEFAULT;
    }

    getkeys_list.Reset();

    const std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(arguments_node, arguments_node.number_elements - 1);
    ASSERT(dimension_values.size() < hashmap.GetNumberDimensions());

    for( const LogicHashMap::Data* const key : hashmap.GetKeys(dimension_values) )
    {
        ASSERT(key != nullptr);

        if( getkeys_list.IsNumeric() )
        {
            ASSERT(std::holds_alternative<double>(*key));
            getkeys_list.AddValue(std::get<double>(*key));
        }

        else
        {
            if( std::holds_alternative<double>(*key) )
            {
                getkeys_list.AddValue<SharableString>(DoubleToString(std::get<double>(*key)));
            }

            else
            {
                getkeys_list.AddValue(std::get<SharableString>(*key));
            }
        }
    }

    return getkeys_list.GetCount();
}
