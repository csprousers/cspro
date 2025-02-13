#include "stdafx.h"
#include "BaseCompiler.h"
#include "BaseCompilerSettings.h"
#include "FunctionTable.h"
#include "KeywordTable.h"
#include "ReservedWords.h"
#include "StringLiteralParser.h"
#include "SymbolTable.h"
#include <zToolsO/VarFuncs.h>
#include <zUtilO/imsaStr.h>

using namespace Logic;


// --------------------------------------------------------------------------
// BaseCompiler
// --------------------------------------------------------------------------

BaseCompiler::BaseCompiler(SymbolTable& symbol_table)
    :   m_symbolTable(symbol_table),
        m_tokensEnd(m_tokens + _countof(m_tokens)),
        m_currentToken(const_cast<Token*>(m_tokensEnd) - 1),
        m_suppressErrorReporting(false),
        m_suppressExceptionsOnInvalidSymbols(false),
        m_preferredSymbolType(SymbolType::None)
{
}


const Token& BaseCompiler::NextTokenWithPreference(SymbolType preferred_symbol_type)
{
    if( preferred_symbol_type == SymbolType::None )
        preferred_symbol_type = m_preferredSymbolType;

    // move the current token to the next one
    if( ++m_currentToken == m_tokensEnd )
        m_currentToken = m_tokens;

    // read and process the next token
    const BasicToken* basic_token = NextBasicToken();

    // quit if at the end of the procedure
    if( basic_token == nullptr )
    {
        m_currentToken->reset(TokenCode::TOKEOP);
        return *m_currentToken;
    }

    // set the token code and string, and then handle special cases
    m_currentToken->reset(basic_token->token_code, basic_token->GetTextSV());

    if( basic_token->type == BasicToken::Type::Operator )
    {
        ProcessOperatorToken();
    }

    else if( basic_token->type == BasicToken::Type::NumericConstant )
    {
        ASSERT(basic_token->token_code == TokenCode::TOKCTE);
        m_currentToken->value = chartodval(m_currentToken->text.c_str(), m_currentToken->text.length(), 0);
    }

    else if( basic_token->type == BasicToken::Type::StringLiteral )
    {
        ASSERT(basic_token->token_code == TokenCode::TOKSCTE);
        ProcessStringLiteralToken();
    }

    else if( basic_token->type == BasicToken::Type::DollarSign || basic_token->type == BasicToken::Type::Text )
    {
        ProcessText(preferred_symbol_type);
    }

    else
    {
        ASSERT(false);
    }

    return *m_currentToken;
}


void BaseCompiler::ProcessOperatorToken()
{
    // conditionally change a minus operator to a minus
    if( m_currentToken->code == TokenCode::TOKMINOP )
    {
        const BasicToken* previous_basic_token = GetPreviousBasicToken();

        if( previous_basic_token != nullptr )
        {
            switch( previous_basic_token->token_code )
            {
                case TokenCode::TOKEQOP:
                case TokenCode::TOKLPAREN:
                case TokenCode::TOKIMPLOP:
                case TokenCode::TOKLBRACK:      // accept negative sign inside table ranges
                case TokenCode::TOKNEOP:
                case TokenCode::TOKLTOP:
                case TokenCode::TOKLEOP:
                case TokenCode::TOKGTOP:
                case TokenCode::TOKGEOP:
                case TokenCode::TOKCOMMA:
                case TokenCode::TOKSEMICOLON:
                case TokenCode::TOKCOLON:       // added for in lists
                    m_currentToken->code = TokenCode::TOKMINUS;
                    break;
            }
        }
    }
}


void BaseCompiler::ProcessStringLiteralToken()
{
    const BasicToken* basic_token = GetCurrentBasicToken();

    m_currentToken->text.clear();

    // loop because multiple string literals listed directly after each other are automatically concatenated together
    while( true )
    {
        StringLiteralParser::Parse(*this, m_currentToken->text, basic_token->GetTextSV());

        // see if the next token is a string literal, or a + with a string literal
        size_t tokens_to_read_before_next_string_literal = 0;
        const BasicToken* next_basic_token = PeekNextBasicToken();

        if( next_basic_token != nullptr )
        {
            if( next_basic_token->type == BasicToken::Type::StringLiteral )
            {
                tokens_to_read_before_next_string_literal = 1;
            }

            else if( next_basic_token->type == BasicToken::Type::Operator && next_basic_token->token_code == TokenCode::TOKADDOP )
            {
                next_basic_token = PeekNextBasicToken(1);

                if( next_basic_token != nullptr && next_basic_token->type == BasicToken::Type::StringLiteral )
                    tokens_to_read_before_next_string_literal = 2;
            }
        }

        if( tokens_to_read_before_next_string_literal == 0 )
            break;

        while( tokens_to_read_before_next_string_literal-- > 0 )
            basic_token = NextBasicToken();
    }
}


void BaseCompiler::ProcessText(SymbolType preferred_symbol_type)
{
    const BasicToken* basic_token = GetCurrentBasicToken();

    // first check if the text is a keyword
    const KeywordDetails* keyword_details;

    if( KeywordTable::IsKeyword(m_currentToken->text, &keyword_details) )
    {
        m_currentToken->code = keyword_details->token_code;
        ProcessKeyword();
        return;
    }

    // read basic tokens until exhausting any names tied together with dot notation
    m_currentToken->text.clear();
    m_textTokensForProcessText.clear();

    while( true )
    {
        if( basic_token->type == BasicToken::Type::DollarSign )
        {
            m_textTokensForProcessText.emplace_back(GetCurrentProcName());
        }

        else
        {
            m_textTokensForProcessText.emplace_back(basic_token->GetTextSV());
        }

        m_currentToken->text.append(m_textTokensForProcessText.back());

        // check if the current text is followed by a dot
        const BasicToken* next_basic_token = PeekNextBasicToken();

        if( next_basic_token != nullptr && next_basic_token->type == BasicToken::Type::Operator && next_basic_token->token_code == TokenCode::TOKPERIOD )
        {
            next_basic_token = PeekNextBasicToken(1);

            if( next_basic_token != nullptr && next_basic_token->type == BasicToken::Type::Text )
            {
                // parse the next part of the dot notation
                basic_token = NextBasicToken();
                m_currentToken->text.append(basic_token->GetTextSV());
                basic_token = NextBasicToken();
                continue;
            }
        }

        break;
    }

    int error_number = 0;

    try
    {
        std::variant<SymbolType, FunctionNamespace> symbol_type_or_function_namespace = SymbolType::None;
        Symbol* symbol = nullptr;

        size_t number_remaining_tokens =  m_textTokensForProcessText.size();

        for( const std::wstring& name : m_textTokensForProcessText )
        {
            --number_remaining_tokens;

            // check if this is a function namespace
            const FunctionNamespaceDetails* function_namespace_details;

            if( FunctionTable::IsFunctionNamespace(name, symbol_type_or_function_namespace, &function_namespace_details) )
            {
                symbol_type_or_function_namespace = function_namespace_details->function_namespace;
            }

            // check if this is a function, or a function for the symbol's wrapped data
            else if( ( FunctionTable::IsFunction(name, symbol_type_or_function_namespace, &m_currentToken->function_details) ) ||
                     ( symbol != nullptr && symbol->GetWrappedType() != SymbolType::None && FunctionTable::IsFunction(name, symbol->GetWrappedType(), &m_currentToken->function_details) ) )
            {
                if( number_remaining_tokens > 0 )
                    IssueError(93005, name.c_str());

                m_currentToken->symbol = symbol;
                ProcessFunction();
                return;
            }

            // if not using dot notation, simply search for the symbol
            else if( m_textTokensForProcessText.size() == 1 )
            {
                symbol = &m_symbolTable.FindSymbol(name, preferred_symbol_type, nullptr);
            }

            // otherwise loop while each symbol is evaluated separately
            else
            {
                // if not a function, check if this is a child of the current symbol
                if( symbol != nullptr )
                {
                    try
                    {
                        symbol = &m_symbolTable.FindSymbol(name, symbol);
                    }

                    catch( const SymbolTable::NoSymbolsException& )
                    {
                        // symbol is still the parent at this point
                        IssueError(93003, symbol->GetName().c_str(), name.c_str());
                    }
                }

                // otherwise get the full set of symbols with this name
                else
                {
                    symbol = &m_symbolTable.FindSymbol(name);
                    MarkSymbolAsUsed(*symbol);
                }

                symbol_type_or_function_namespace = symbol->GetType();
            }

            // if checking case, check against the symbol name and any aliases
            if( symbol != nullptr && GetLogicSettings().CaseSensitiveSymbols() )
                CheckSymbolCase(*symbol, name);
        }

        // if symbol isn't defined, it means that a function namespace was processed without a function
        if( symbol == nullptr )
        {
            error_number = 93004;
        }

        else
        {
            m_currentToken->symbol = symbol;
            ProcessSymbol();
            return;
        }
    }

    catch( const SymbolTable::Exception& exception )
    {
        error_number = exception.GetCompilerErrorMessageNumber();
    }

    // when the text does not match anything, an error can be suppressed (e.g., adding a new symbol name)
    if( ShouldSuppressExceptionsOnInvalidSymbols() )
    {
        m_currentToken->code = TokenCode::TOKERROR;
        return;
    }

    // or, when no symbols were found, we can check if there is a symbol name or reserved word that nearly matches
    if( error_number == 93000 )
    {
        const std::wstring fuzzy_matched_word = m_symbolTable.GetRecommendedWordUsingFuzzyMatching(m_currentToken->text);

        if( !fuzzy_matched_word.empty() )
            IssueError(93007, m_currentToken->text.c_str(), fuzzy_matched_word.c_str());
    }

    // otherwise issue the generic error
    IssueError(error_number, m_currentToken->text.c_str());
}


void BaseCompiler::ProcessKeyword()
{
    // some keywords are converted to constants
    const std::optional<double> keyword_constant = KeywordTable::GetKeywordConstant(m_currentToken->code);

    if( keyword_constant.has_value() )
    {
        m_currentToken->value = *keyword_constant;
        m_currentToken->code = TokenCode::TOKCTE;
    }
}


void BaseCompiler::ProcessSymbol()
{
    m_currentToken->code = GetTokenCodeFromSymbolType(m_currentToken->symbol->GetType());
}


TokenCode BaseCompiler::GetTokenCodeFromSymbolType(SymbolType symbol_type)
{
    static const std::map<SymbolType, TokenCode> symbol_to_token_code_map =
    {
        { SymbolType::Application,     TokenCode::TOKERROR },
        { SymbolType::Array,           TokenCode::TOKARRAY },
        { SymbolType::Audio,           TokenCode::TOKAUDIO },
        { SymbolType::Block,           TokenCode::TOKBLOCK },
        { SymbolType::Crosstab,        TokenCode::TOKCROSSTAB },
        { SymbolType::Dictionary,      TokenCode::TOKDICT },
        { SymbolType::Pre80Dictionary, TokenCode::TOKDICT_PRE80 },
        { SymbolType::Document,        TokenCode::TOKDOCUMENT },
        { SymbolType::File,            TokenCode::TOKFILE },
        { SymbolType::Flow,            TokenCode::TOKFLOW },
        { SymbolType::Pre80Flow,       TokenCode::TOKFLOW_PRE80 },
        { SymbolType::Form,            TokenCode::TOKFORM },
        { SymbolType::Geometry,        TokenCode::TOKGEOMETRY },
        { SymbolType::Group,           TokenCode::TOKGROUP },
        { SymbolType::HashMap,         TokenCode::TOKHASHMAP },
        { SymbolType::Image,           TokenCode::TOKIMAGE },
        { SymbolType::Item,            TokenCode::TOKITEM },
        { SymbolType::List,            TokenCode::TOKLIST },
        { SymbolType::Map,             TokenCode::TOKMAP },
        { SymbolType::NamedFrequency,  TokenCode::TOKFREQ },
        { SymbolType::Pff,             TokenCode::TOKPFF },
        { SymbolType::Record,          TokenCode::TOKRECORD },
        { SymbolType::Relation,        TokenCode::TOKRELATION },
        { SymbolType::Report,          TokenCode::TOKREPORT },
        { SymbolType::Section,         TokenCode::TOKSECT },
        { SymbolType::SystemApp,       TokenCode::TOKSYSTEMAPP },
        { SymbolType::UserFunction,    TokenCode::TOKUSERFUNCTION },
        { SymbolType::Variable,        TokenCode::TOKVAR },
        { SymbolType::ValueSet,        TokenCode::TOKVALUESET },
        { SymbolType::WorkString,      TokenCode::TOKWORKSTRING },
        { SymbolType::WorkVariable,    TokenCode::TOKVAR },
    };

    const auto& lookup = symbol_to_token_code_map.find(symbol_type);
    ASSERT(lookup != symbol_to_token_code_map.cend());
    return lookup->second;
}


void BaseCompiler::ProcessFunction()
{
    m_currentToken->code = TokenCode::TOKFUNCTION;
}


void BaseCompiler::CheckSymbolCase(const Symbol& symbol, const std::wstring& compiled_case)
{
    if( symbol.GetName() == compiled_case )
        return;

    // issue an error if the case matches the symbol but with a different case
    if( SO::EqualsNoCase(compiled_case, symbol.GetName()) )
        IssueError(93006, compiled_case.c_str(), symbol.GetName().c_str());

    // check if this is an alias
    std::vector<std::wstring> aliases = m_symbolTable.GetAliases(symbol);
    const auto& alias_lookup = std::find_if(aliases.cbegin(), aliases.cend(),
                                            [&](const std::wstring& alias) { return SO::EqualsNoCase(alias, compiled_case); });

    if( alias_lookup != aliases.cend() )
    {
        // issue an error if the alias case does not match
        if( compiled_case != *alias_lookup )
            IssueError(93006, compiled_case.c_str(), alias_lookup->c_str());
    }

    else
    {
        // if here, this is a constructed symbol like valueset_name.codes
        size_t dot_pos = symbol.GetName().find_last_of('.');

        if( dot_pos != std::wstring::npos )
        {
            wstring_view last_name_sv = wstring_view(symbol.GetName()).substr(dot_pos + 1);

            if( compiled_case != last_name_sv )
                IssueError(93006, compiled_case.c_str(), std::wstring(last_name_sv).c_str());
        }

        else
        {
            ASSERT(false);
        }
    }
}


const Token& BaseCompiler::NextTokenOrNewSymbolName()
{
    BaseCompilerSettings compiler_settings_modifier = ModifyCompilerSettings();
    compiler_settings_modifier.SuppressExceptionsOnInvalidSymbols();

    NextToken();

    // if the token was not matched to anything else, then process it as a new symbol name
    if( m_currentToken->code == TokenCode::TOKERROR )
    {
        CheckIfValidNewSymbolName(m_currentToken->text);
        m_currentToken->code = TokenCode::TOKNEWSYMBOL;
    }

    return *m_currentToken;
}


void BaseCompiler::CheckIfValidNewSymbolName(const std::wstring& new_symbol_name)
{
    // check that the name is valid
    if( !CIMSAString::IsName(new_symbol_name) )
        IssueError(101, new_symbol_name.c_str());

    // check if the name is already in use
    if( m_symbolTable.NameExists(new_symbol_name) )
        IssueError(102, new_symbol_name.c_str());

    // check if this is a reserved name
    if( ReservedWords::IsReservedWord(new_symbol_name) )
        IssueError(162, new_symbol_name.c_str());
}


void BaseCompiler::IssueErrorOnTokenMismatch(const Token& _token, TokenCode token_code, int message_number)
{
    if( _token.code != token_code )
        IssueError(message_number);
}


template<typename T/* = const TCHAR**/>
size_t BaseCompiler::NextKeywordOrError(const std::vector<T>& keywords)
{
    size_t keyword_type = NextKeyword(keywords);

    if( keyword_type == 0 )
        IssueError(7016, SO::CreateSingleString(keywords).c_str());

    return keyword_type;
}

template ZLOGICO_API size_t BaseCompiler::NextKeywordOrError(const std::vector<const TCHAR*>& keywords);
template ZLOGICO_API size_t BaseCompiler::NextKeywordOrError(const std::vector<std::wstring>& keywords);


bool BaseCompiler::NextKeywordIf(TokenCode token_code)
{
    if( IsNextToken(token_code) )
    {
        NextToken();
        ASSERT(m_currentToken->code == token_code);
        return true;
    }

    return false;
}


bool BaseCompiler::IsNextToken(cs::span<const TokenCode> token_codes) const
{
    const BasicToken* next_basic_token = PeekNextBasicToken();

    if( next_basic_token != nullptr )
    {
        std::optional<TokenCode> next_token_code;
        const KeywordDetails* keyword_details;

        if( next_basic_token->type == BasicToken::Type::Operator )
        {
            next_token_code = next_basic_token->token_code;
        }

        else if( KeywordTable::IsKeyword(next_basic_token->GetTextSV(), &keyword_details) )
        {
            next_token_code = keyword_details->token_code;
        }

        if( next_token_code.has_value() &&
            std::find(token_codes.begin(), token_codes.end(), *next_token_code) != token_codes.end() )
        {
            return true;
        }
    }

    return false;
}


bool BaseCompiler::IsNextTokenNamedArgument() const
{
    const Logic::BasicToken* two_tokens_from_here = PeekNextBasicToken(1);

    return ( two_tokens_from_here != nullptr &&
             two_tokens_from_here->token_code == TokenCode::TOKNAMEDARGOP );
}


BaseCompilerSettings BaseCompiler::ModifyCompilerSettings()
{
    return BaseCompilerSettings(*this);
}
