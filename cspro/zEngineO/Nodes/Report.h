#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    namespace Report
    {
        struct Save
        {
            FunctionCode function_code;
            int symbol_index;
            int filename_expression;
        };


        struct View
        {
            FunctionCode function_code;
            int symbol_index;
            int viewer_options_node_index;
        };


        struct Write
        {
            enum class Type : int { ReportText = 1, TextFill, Write };

            FunctionCode function_code;
            int symbol_index;
            Type type;
            int expression;
            int encode_text; // 0 = false, 1 = true
        };
    }
}
