#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Impute
    {
        FunctionCode function_code;
        int imputation_index;
        int variable_compilation;
        int value_expression;
        int title_expression;
        int stat_variable_compilation_list_node;
    };
}
