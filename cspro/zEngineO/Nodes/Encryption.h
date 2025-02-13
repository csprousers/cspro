#pragma once

#include <zToolsO/Encryption.h>
#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct Encryption
    {
        FunctionCode function_code;
        int next_st; // unused at the moment
        int string_expression;
        Encryptor::Type encryption_type;
    };
}
