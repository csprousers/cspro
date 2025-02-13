#pragma once

#include <zLogicO/FunctionTable.h>
#include <zLogicO/GeneralizedFunction.h>


namespace Nodes
{
    struct GeneralizedFunctionValue
    {
        GF::VariableType parameter_variable_type;
        GF::VariableType argument_variable_type;
        int argument_expression;
    };
}
