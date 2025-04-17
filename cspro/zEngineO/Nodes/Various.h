#pragma once

#include <zUtilO/DataTypes.h>
#include <zLogicO/FunctionTable.h>

enum class EncodeType : int;
namespace Nodes { struct Connection; struct Encode; struct Hash; }


struct Nodes::Connection
{
    static constexpr int Mobile = 0x00000001;
    static constexpr int WiFi   = 0x00000002;
    static constexpr int Any    = 0xFFFFFFFF;

    FunctionCode function_code;
    int connection_type;
};


struct Nodes::Encode
{
    FunctionCode function_code;
    EncodeType encode_type;
    int string_expression;
};


struct Nodes::Hash
{
    FunctionCode function_code;
    DataType value_data_type;
    int value_expression;
    int length_expression;
    int salt_expression;
};
