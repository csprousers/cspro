#pragma once

#include <zUtilO/DataTypes.h>
#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Timestamp
    {
        enum class Type
        {
            Current,
            RFC3339,
            SpecifiedDate
        };

        FunctionCode function_code;
        Type type;
        int argument;
    };
}
