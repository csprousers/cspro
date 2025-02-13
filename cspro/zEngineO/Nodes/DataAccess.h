#pragma once

#include <zUtilO/DataTypes.h>
#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct DataAccessValidityCheck
    {
        FunctionCode function_code;
        int program_index;
        int symbol_index;
        int symbol_level_number_base1;
        DataType function_return_data_type;
    };
}
