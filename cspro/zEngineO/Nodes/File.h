#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct File
    {
        FunctionCode function_code;
        int symbol_index_or_string_expression; // negative for file and dictionary symbols
        int elements_list_node;
    };


    struct SetFile
    {
        enum class Mode : int { Update = 0, Create = 1, Append = 2 };

        FunctionCode function_code;
        int symbol_index;
        int filename_expression;
        Mode mode;
    };
}
