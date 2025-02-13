#pragma once

#include <zUtilO/DataTypes.h>
#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct CreateQRCode
    {
        FunctionCode function_code;
        int symbol_index_or_filename_expression; // this is a symbol if < 0
        int subscript_compilation;
        DataType value_data_type;
        int value_expression;
        int options_node_index;
    };


    struct CreateQRCodeOptions
    {
        int error_correction_expression;
        int scale_expression;
        int quiet_zone_expression;
        int dark_color_expression;
        int light_color_expression;
    };
}
