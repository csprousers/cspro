#pragma once

#include <zLogicO/Token.h>

namespace Logic { struct BasicToken; }


// the source buffer is tokenized into the following basic tokens

struct Logic::BasicToken
{
    enum class Type
    {
        // good tokens
        Operator,
        NumericConstant,
        StringLiteral,
        Text,
        DollarSign,

        // bad tokens
        UnbalancedComment
    };

    Type type;
    TokenCode token_code;
    size_t token_length;
    size_t line_number;      // 1-based
    size_t position_in_line; // 0-based
    const char* token_text;

    std::string GetText() const    { return std::string(token_text, token_length); }
    std::string_view GetSV() const { return std::string_view(token_text, token_length); }
};
