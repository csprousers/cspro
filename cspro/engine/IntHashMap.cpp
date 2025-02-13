#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/HashMap.h>
#include <zEngineO/List.h>
#include <zEngineO/SubscriptText.h>


namespace
{
    std::vector<LogicHashMap::Data> EvaluateDimensionValues(CIntDriver& int_driver,
                                                            const Nodes::List& dimension_expressions_node,
                                                            int number_dimension_expressions)
    {
        std::vector<LogicHashMap::Data> dimension_values;

        for( int i = 0; i < number_dimension_expressions; i += 2 )
        {
            dimension_values.emplace_back(int_driver.EvaluateVariantExpression(static_cast<DataType>(dimension_expressions_node.elements[i]),
                                                                               dimension_expressions_node.elements[i + 1]));
        }

        return dimension_values;
    }


    std::vector<LogicHashMap::Data> EvaluateDimensionValues(CIntDriver& int_driver, const Nodes::List& dimension_expressions_node)
    {
        return EvaluateDimensionValues(int_driver, dimension_expressions_node, dimension_expressions_node.number_elements);
    }
}


std::vector<LogicHashMap::Data> CIntDriver::EvaluateHashMapIndex(int hashmap_node_expression, LogicHashMap** out_hashmap, bool bounds_checking)
{
    const auto& element_reference_node = GetNode<Nodes::ElementReference>(hashmap_node_expression);
    LogicHashMap*& hashmap = *out_hashmap;
    ASSERT(element_reference_node.function_code == HASHMAPVAR_CODE);

    hashmap = &GetSymbolLogicHashMap(element_reference_node.symbol_index);

    std::vector<LogicHashMap::Data> dimension_values = EvaluateDimensionValues(*this, GetListNode(element_reference_node.element_expressions[0]));
    ASSERT(dimension_values.size() == hashmap->GetNumberDimensions());

    if( bounds_checking && !hashmap->HasDefaultValue() && !hashmap->Contains(dimension_values) )
    {
        issaerror(MessageType::Error, 1008, hashmap->GetName().c_str(), GetSubscriptText(dimension_values).c_str());
        dimension_values.clear();
    }

    return dimension_values;
}


double CIntDriver::exhashmapvar(int iExpr)
{
    const LogicHashMap* hashmap;
    std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(iExpr, const_cast<LogicHashMap**>(&hashmap), true);

    if( dimension_values.empty() )
        return AssignInvalidValue(hashmap->GetValueType());

    const std::optional<LogicHashMap::Data>& value = hashmap->GetValue(dimension_values);
    ASSERT(value.has_value());

    return AssignVariantValue(*value);
}


double CIntDriver::exhashmapcompute(int iExpr)
{
    const auto& symbol_compute_node = GetNode<Nodes::SymbolCompute>(iExpr);

    // assigning to an element
    if( symbol_compute_node.rhs_symbol_type == SymbolType::None )
    {
        LogicHashMap* hashmap;
        std::vector<LogicHashMap::Data> dimension_values = EvaluateHashMapIndex(symbol_compute_node.lhs_symbol_index, &hashmap, false);

        if( hashmap->IsValueTypeNumeric() )
        {
            double value = evalexpr(symbol_compute_node.rhs_symbol_index);
            hashmap->SetValue(dimension_values, value);
            return value;
        }

        else
        {
            hashmap->SetValue(dimension_values, EvalAlphaExpr(symbol_compute_node.rhs_symbol_index));
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


double CIntDriver::exhashmapclear(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    hashmap.Reset();

    return 1;
}


double CIntDriver::exhashmapcontains(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    std::vector<LogicHashMap::Data> dimension_values = EvaluateDimensionValues(*this, GetListNode(symbol_va_node.arguments[0]));
    ASSERT(!dimension_values.empty() && dimension_values.size() <= hashmap.GetNumberDimensions());

    return hashmap.Contains(dimension_values) ? 1 : 0;
}


double CIntDriver::exhashmaplength(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    std::vector<LogicHashMap::Data> dimension_values = EvaluateDimensionValues(*this, GetListNode(symbol_va_node.arguments[0]));
    ASSERT(dimension_values.size() < hashmap.GetNumberDimensions());

    return hashmap.GetLength(dimension_values);
}


double CIntDriver::exhashmapremove(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    std::vector<LogicHashMap::Data> dimension_values = EvaluateDimensionValues(*this, GetListNode(symbol_va_node.arguments[0]));
    ASSERT(!dimension_values.empty() && dimension_values.size() <= hashmap.GetNumberDimensions());

    return hashmap.Remove(dimension_values) ? 1 : 0;
}


double CIntDriver::exhashmapgetkeys(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicHashMap& hashmap = GetSymbolLogicHashMap(symbol_va_node.symbol_index);

    const Nodes::List& arguments_node = GetListNode(symbol_va_node.arguments[0]);
    ASSERT(arguments_node.number_elements > 0);

    LogicList& getkeys_list = GetSymbolLogicList(arguments_node.elements[arguments_node.number_elements - 1]);

    if( getkeys_list.IsReadOnly() )
    {
        issaerror(MessageType::Error, 965, getkeys_list.GetName().c_str());
        return DEFAULT;
    }

    getkeys_list.Reset();

    std::vector<LogicHashMap::Data> dimension_values = EvaluateDimensionValues(*this, arguments_node, arguments_node.number_elements - 1);
    ASSERT(dimension_values.size() < hashmap.GetNumberDimensions());

    for( const LogicHashMap::Data* key : hashmap.GetKeys(dimension_values) )
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
                getkeys_list.AddString(DoubleToString(std::get<double>(*key)));
            }

            else
            {
                getkeys_list.AddString(std::get<std::wstring>(*key));
            }
        }
    }

    return getkeys_list.GetCount();
}
