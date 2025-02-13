#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/Array.h>
#include <zEngineO/SubscriptText.h>


std::vector<size_t> CIntDriver::EvaluateArrayIndex(int arrayvar_node_expression, LogicArray** out_logic_array)
{
    const auto& element_reference_node = GetNode<Nodes::ElementReference>(arrayvar_node_expression);
    LogicArray*& logic_array = *out_logic_array;
    std::vector<size_t> indices;

    logic_array = &GetSymbolLogicArray(element_reference_node.symbol_index);

    for( size_t i = 0; i < logic_array->GetNumberDimensions(); ++i )
        indices.emplace_back(evalexpr<size_t>(element_reference_node.element_expressions[i]));

    if( !logic_array->IsValidIndex(indices) )
    {
        issaerror(MessageType::Error, 1008, logic_array->GetName().c_str(), GetSubscriptText(indices).c_str());
        indices.clear();
    }

    return indices;
}


double CIntDriver::exarrayvar(int iExpr)
{
    const LogicArray* logic_array;
    std::vector<size_t> indices = EvaluateArrayIndex(iExpr, const_cast<LogicArray**>(&logic_array));

    if( indices.empty() )
    {
        return AssignInvalidValue(logic_array->IsNumeric() ? DataType::Numeric : DataType::String);
    }

    else if( logic_array->IsNumeric() )
    {
        return logic_array->GetValue<double>(indices);
    }

    else
    {
        return AssignAlphaValue(logic_array->GetValue<std::wstring>(indices));
    }
}


double CIntDriver::exarrayclear(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    LogicArray& logic_array = GetSymbolLogicArray(symbol_va_node.symbol_index);

    logic_array.Reset();

    return 1;
}


double CIntDriver::exarraylength(int iExpr)
{
    const auto& symbol_va_node = GetNode<Nodes::SymbolVariableArguments>(iExpr);
    const LogicArray& logic_array = GetSymbolLogicArray(symbol_va_node.symbol_index);
    size_t dimension = evalexpr<size_t>(symbol_va_node.arguments[0]);

    if( dimension < 1 || dimension > logic_array.GetNumberDimensions() )
    {
        issaerror(MessageType::Error, 19041, logic_array.GetName().c_str(), static_cast<int>(dimension));
        return DEFAULT;
    }

    // don't count the 0th element in the dimension size
    return logic_array.GetDimension(dimension - 1) - 1;
}
