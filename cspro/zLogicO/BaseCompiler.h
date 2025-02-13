#pragma once

#include <zLogicO/zLogicO.h>
#include <zLogicO/BasicTokenCompiler.h>
#include <zLogicO/SymbolType.h>
#include <zLogicO/Token.h>


namespace Logic
{
    class SymbolTable;
    class BaseCompilerSettings;


    class ZLOGICO_API BaseCompiler : public BasicTokenCompiler
    {
        friend BaseCompilerSettings;

    public:
        BaseCompiler(SymbolTable& symbol_table);

        Token& GetCurrentToken() const { return *m_currentToken; } // LOGIC_TODO eventually return: const Token&

        // Gets and processes the next token.
        const Token& NextToken() { return NextTokenWithPreference(SymbolType::None); }

        // Gets and processes the next token using a preferred symbol type.
        const Token& NextTokenWithPreference(SymbolType preferred_symbol_type);

        // Gets and processes the next token and potentially interprets it as a new symbol name,
        // verifying that it is a unique and valid name.
        const Token& NextTokenOrNewSymbolName();

        // Checks that a name is a valid new symbol name.
        void CheckIfValidNewSymbolName(const std::wstring& new_symbol_name);

        // Issues an error if the current token's code does not match.
        void IssueErrorOnTokenMismatch(const Token& _token, TokenCode token_code, int message_number);

        // Issues an error if the current token's code does not match.
        void IssueErrorOnTokenMismatch(TokenCode token_code, int message_number) { return IssueErrorOnTokenMismatch(*m_currentToken, token_code, message_number); }

        // Checks if the next token is in the list of keywords. If found, the function returns
        // the 1-based index of the selected and the compiler advances past the keyword.
        // If not found, the function issues an error indicating the valid keyword options.
        template<typename T = const TCHAR*>
        size_t NextKeywordOrError(const std::vector<T>& keywords);

        // Returns true and reads the next token if it matches the single supplied keyword,
        // specified as text. This method is similar to calling NextKeyword with the
        // single keyword and checking whether the method returned 1.
        bool NextKeywordIf(const TCHAR* keyword) { return ( NextKeyword({ keyword }) == 1 ); }

        // Returns true and reads the next token if it matches the single supplied keyword,
        // specified as a token code. The token code must be an operator or must have an entry
        // in the keyword table. The token will be fully processed when it matches.
        bool NextKeywordIf(TokenCode token_code);

        // Returns true if the next token matches the supplied keyword(s), specified as
        // token code(s). The token code(s) must be operators or must have entries
        // in the keyword table.
        bool IsNextToken(cs::span<const TokenCode> token_codes) const;

        // Returns true if the next token is a named argument.
        bool IsNextTokenNamedArgument() const;

        // Returns an object that can be used to modify the compiler settings.
        BaseCompilerSettings ModifyCompilerSettings();

        bool ShouldSuppressErrorReporting() const               { return m_suppressErrorReporting; }
        bool ShouldSuppressExceptionsOnInvalidSymbols() const   { return m_suppressExceptionsOnInvalidSymbols; }

    private:
        void ProcessOperatorToken();
        void ProcessStringLiteralToken();
        void ProcessText(SymbolType preferred_symbol_type);
        void ProcessKeyword();

    protected:
        virtual void MarkSymbolAsUsed(Symbol& symbol) { symbol; }
        virtual void ProcessSymbol();
        static TokenCode GetTokenCodeFromSymbolType(SymbolType symbol_type);

        virtual void ProcessFunction();

    private:
        void CheckSymbolCase(const Symbol& symbol, const std::wstring& compiled_case);

    protected:
        SymbolTable& m_symbolTable;

    private:
        static constexpr size_t NumberTokensToBuffer = 200;
        Token m_tokens[NumberTokensToBuffer];
        const Token* m_tokensEnd;
        Token* m_currentToken;
        std::vector<std::wstring> m_textTokensForProcessText;

        // some settings
        bool m_suppressErrorReporting;
        bool m_suppressExceptionsOnInvalidSymbols;
        SymbolType m_preferredSymbolType;
    };
}


#define CurrentToken    (GetCurrentToken())
#define Tkn             CurrentToken.code
#define Tokstr          CurrentToken.text
#define Tokvalue        CurrentToken.value
