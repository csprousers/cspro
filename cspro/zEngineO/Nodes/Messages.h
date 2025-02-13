#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Message
    {
        FunctionCode function_code;
        int message_number;
        int message_expression;
        int argument_list;
        int extended_message_node_index;
    };


    struct ExtendedMessage
    {
        enum class DisplayType { Default, Summary, Case };

        int denominator_symbol_index;
        DisplayType display_type;

        int select_button_texts_list;
        int select_movements_list;
        int select_default_button_expression;
    };


    struct VariableValue
    {
        FunctionCode function_code;
        int symbol_index;
        int expression;
    };
}
