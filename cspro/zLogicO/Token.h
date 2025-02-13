#pragma once

#include <zLogicO/TokenCode.h>

class Symbol;

namespace Logic { struct FunctionDetails; }


namespace Logic
{
    struct Token
    {
        TokenCode code;
        std::wstring text;
        double value;
        const FunctionDetails* function_details;
        Symbol* symbol;
        int symbol_subscript_compilation;

    public:
        Token();

        void reset(TokenCode code_);
        void reset(TokenCode code_, wstring_view text_sv);

    private:
        void reset_all_but_text(TokenCode code_);
    };
}


inline Logic::Token::Token()
{
    reset_all_but_text(TokenCode::Unspecified);
}


inline void Logic::Token::reset_all_but_text(TokenCode code_)
{
    code = code_;
    value = 0;
    function_details = nullptr;
    symbol = nullptr;
    symbol_subscript_compilation = -1;
}


inline void Logic::Token::reset(TokenCode code_)
{
    reset_all_but_text(code_);
    text.clear();
}


inline void Logic::Token::reset(TokenCode code_, wstring_view text_sv)
{
    reset_all_but_text(code_);
    text = text_sv;
}
