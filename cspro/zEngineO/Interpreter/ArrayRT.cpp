#include "stdafx.h"
#include "IncludesRT.h"
#include "Array.h"
#include "SubscriptText.h"


std::vector<size_t> LogicInterpreter::EvaluateArrayIndex(const int arrayvar_node_expression, LogicArray** const out_logic_array)
{
    const auto& element_reference_node = GetNode<Nodes::ElementReference>(arrayvar_node_expression);
    LogicArray*& logic_array = *out_logic_array;
    std::vector<size_t> indices;

    logic_array = &GetSymbolLogicArray(element_reference_node.symbol_index);

    for( size_t i = 0; i < logic_array->GetNumberDimensions(); ++i )
        indices.emplace_back(Evaluate<size_t>(element_reference_node.element_expressions[i]));

    if( !logic_array->IsValidIndex(indices) )
    {
        IssueMessage(MessageType::Error, MGF::invalid_subscript_1008, logic_array->GetName().c_str(), GetSubscriptText(indices).c_str());
        indices.clear();
    }

    return indices;
}


double LogicInterpreter::ex_Array_var(const int program_index)
{
    const LogicArray* logic_array;
    const std::vector<size_t> indices = EvaluateArrayIndex(program_index, const_cast<LogicArray**>(&logic_array));

    if( indices.empty() )
    {
        return AssignInvalidValue(logic_array->GetDataType());
    }

    else if( logic_array->IsNumeric() )
    {
        return logic_array->GetValue<double>(indices);
    }

    else
    {
        return AssignString(logic_array->GetValue<SharableString>(indices));
    }
}


double LogicInterpreter::ex_Array_compute(const int program_index)
{
    const auto& symbol_compute_expression_node = GetNode<Nodes::SymbolComputeExpression>(program_index);
    const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(symbol_compute_expression_node.symbol_value_node_index);

    LogicArray* logic_array;
    const std::vector<size_t> indices = EvaluateArrayIndex(symbol_value_node.symbol_compilation, &logic_array);
    ASSERT(logic_array == &NPT_Ref(symbol_value_node.symbol_index));

    if( indices.empty() )
        return DEFAULT;

    const double value = Evaluate(symbol_compute_expression_node.rhs_expression);

    logic_array->SetValue(indices, value);

    return value;
}


double LogicInterpreter::ex_Array_clear(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);
    LogicArray& logic_array = GetSymbolLogicArray(symbol_va_node.symbol_index);

    logic_array.Reset();

    return 1;
}


double LogicInterpreter::ex_Array_length(const int program_index)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(program_index);

    return ex_Array_length(GetSymbolLogicArray(symbol_va_node.symbol_index),
                           Evaluate<size_t>(symbol_va_node.arguments[0]));
}


double LogicInterpreter::ex_Array_length(const LogicArray& logic_array, const size_t dimension)
{
    if( dimension < 1 || dimension > logic_array.GetNumberDimensions() )
    {
        IssueMessage(MessageType::Error, MGF::Array_invalid_dimension_19041, logic_array.GetName().c_str(), static_cast<int>(dimension));
        return DEFAULT;
    }

    // don't count the 0th element in the dimension size
    return static_cast<double>(logic_array.GetDimension(dimension - 1) - 1);
}
