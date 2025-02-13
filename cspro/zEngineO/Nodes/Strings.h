#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct StringCompute
    {
        FunctionCode function_code;
        int next_st;
        int symbol_value_node_index;
        int substring_index_expression;
        int substring_length_expression;
        int string_expression;
    };


    struct StringExpression
    {
        FunctionCode function_code;
        int string_expression; // >= 0 if a direct string expression
        int substring_index_expression;
        int substring_length_expression;
    };


    struct StringLiteral
    {
        int st_code;
        int string_literal_index;
    };


    struct TextFill
    {
        DataType data_type;
        int symbol_index_or_expression;
        int subscript_compilation;
    };
}
