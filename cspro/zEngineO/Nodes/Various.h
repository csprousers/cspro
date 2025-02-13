#pragma once

#include <zUtilO/DataTypes.h>
#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Connection
    {
        static constexpr int Mobile = 0x00000001;
        static constexpr int WiFi   = 0x00000002;
        static constexpr int Any    = 0xFFFFFFFF;

        FunctionCode function_code;
        int connection_type;
    };


    enum class EncodeType : int { Default, Html, Csv, PercentEncoding, Uri, UriComponent, Slashes, JsonString };
    
    struct Encode
    {
        FunctionCode function_code;
        EncodeType encoding_type;
        int string_expression;
    };


    struct Hash
    {
        FunctionCode function_code;
        DataType value_data_type;
        int value_expression;
        int length_expression;
        int salt_expression;
    };
}
