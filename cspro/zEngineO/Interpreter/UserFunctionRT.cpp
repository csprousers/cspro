#include "stdafx.h"
#include "IncludesRT.h"
#include "UserFunction.h"


Engine::Value LogicInterpreter::ex_UserFunction_compute(const int program_index)
{
    const auto& symbol_compute_expression_node = GetNode<Nodes::SymbolComputeExpression>(program_index);
    const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(symbol_compute_expression_node.symbol_value_node_index);
    ASSERT(symbol_value_node.symbol_compilation == -1);

    UserFunction& user_function = GetSymbolUserFunction(symbol_value_node.symbol_index);

    // string assignments are handled in ex_string_compute
    ASSERT(IsNumeric(user_function.GetReturnDataType()));

    const double value = Evaluate<double>(symbol_compute_expression_node.rhs_expression);

    user_function.SetReturnValue(value);

    return value;
}
