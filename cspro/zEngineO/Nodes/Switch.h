#pragma once


namespace Nodes
{
    struct In
    {
        FunctionCode function_code;
        DataType data_type;
        int left_expr;
        int right_expr;

        struct Entry
        {
            int expression_low;
            int expression_high;
            int next_entry_index;
        };
    };


    struct Switch
    {
        FunctionCode function_code;
        int next_st;
        int number_condition_values;
        int number_destinations;
        int number_actions;
        int expressions[1];
    };
}
