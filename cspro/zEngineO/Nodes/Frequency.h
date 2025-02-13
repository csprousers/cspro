#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct UnnamedFrequency
    {
        FunctionCode function_code;
        int next_st;
        int frequency_index;
        int universe_expression;
        int weight_expression;
        int heading_expressions_list_node;
    };


    struct FrequencyParameters
    {
        int heading_expressions_list_node;
        int distinct;
        int value_sets_list_node;
        int use_all_value_sets;
        int show_statistics;
        int show_no_frequencies;
        int percentiles;
        int show_no_net_percents;
        int decimals;
        int page_length;
        int sort_order_and_type;
    };
}
