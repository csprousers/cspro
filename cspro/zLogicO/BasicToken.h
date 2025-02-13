#pragma once

#include <zLogicO/Token.h>


namespace Logic
{
    // the source buffer is tokenized into the following basic tokens
    struct BasicToken
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
        size_t line_number;             // 1-based
        size_t position_in_line;        // 0-based
        const TCHAR* token_text;

        std::wstring GetText() const   { return std::wstring(token_text, token_length); }
        wstring_view GetTextSV() const { return wstring_view(token_text, token_length); }
    };
}
