#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Do
    {
        FunctionCode function_code;
        int next_st;
        int loop_type;
        int counter_symbol_value_node_index;
        int counter_initial_value_expression;
        int counter_increment_by_expression;
        int conditional_expression;
        int block_program_index;
    };


    struct If
    {
        FunctionCode function_code;
        int next_st;
        int conditional_expression;
        int then_program_index;
        int else_program_index;
    };


    struct While
    {
        FunctionCode function_code;
        int next_st;
        int conditional_expression;
        int block_program_index;
    };
}
