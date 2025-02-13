#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Invoke
    {
        FunctionCode function_code;
        int function_name_expression;
        int arguments_expression;
        int arguments_list;
    };


    struct UserFunction
    {
        FunctionCode function_code;
        int user_function_symbol_index;
        int reference_destinations_list;
        int argument_expressions[1];
    };
}
