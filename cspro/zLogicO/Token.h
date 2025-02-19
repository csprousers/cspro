#pragma once

#include <zLogicO/TokenCode.h>

class Symbol;

namespace Logic { struct FunctionDetails; struct Token; }


struct Logic::Token
{
    TokenCode code;
    std::string text;
    double value;
    const FunctionDetails* function_details;
    Symbol* symbol;
    int symbol_subscript_compilation;

public:
    Token();

    void reset(TokenCode code_);

    template<typename T>
    void reset(TokenCode code_, T&& text_);

private:
    void reset_all_but_text(TokenCode code_);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Logic::Token::Token()
{
    reset_all_but_text(TokenCode::Unspecified);
}


inline void Logic::Token::reset_all_but_text(const TokenCode code_)
{
    code = code_;
    value = 0;
    function_details = nullptr;
    symbol = nullptr;
    symbol_subscript_compilation = -1;
}


inline void Logic::Token::reset(const TokenCode code_)
{
    reset_all_but_text(code_);
    text.clear();
}


template<typename T>
void Logic::Token::reset(const TokenCode code_, T&& text_)
{
    reset_all_but_text(code_);
    text = std::forward<T>(text_);
}
