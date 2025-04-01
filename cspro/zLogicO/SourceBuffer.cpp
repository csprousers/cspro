#include "stdafx.h"
#include "SourceBuffer.h"
#include <zToolsO/EscapesAndLogicOperators.h>

using namespace Logic;


namespace
{
    constexpr TokenCode SingleCharacterOperatorTokenCodes[17] =
    {
        TokenCode::TOKADDOP,
        TokenCode::TOKMULOP,
        TokenCode::TOKDIVOP,
        TokenCode::TOKMODOP,
        TokenCode::TOKEXPOP,
        TokenCode::TOKLPAREN,
        TokenCode::TOKRPAREN,
        TokenCode::TOKLBRACK,
        TokenCode::TOKRBRACK,
        TokenCode::TOKANDOP,
        TokenCode::TOKOROP,
        TokenCode::TOKNOTOP,
        TokenCode::TOKCOMMA,
        TokenCode::TOKSEMICOLON,
        TokenCode::TOKATOP,
        TokenCode::TOKPERIOD,
        TokenCode::TOKHASH,
    };
}


// --------------------------------------------------------------------------
// SourceBufferTokenizer
// --------------------------------------------------------------------------

class Logic::SourceBufferTokenizer
{
public:
    SourceBufferTokenizer(std::string_view buffer_sv, std::vector<BasicToken>& basic_tokens, const LogicSettings& logic_settings);

    void Tokenize();

private:
    char NextChar();

    bool AdditionalCharactersExistInBuffer() const
    {
        return ( m_currentBufferPosition < m_bufferEndPosition );
    }

    bool AdditionalCharactersExistInBuffer(const size_t required_length) const
    {
        return ( ( m_currentBufferPosition + required_length ) <= m_bufferEndPosition );
    }

    bool CheckFullMatchingText(std::string_view text_sv, bool advance_past_characters);

    void AddTokenFromCurrentPosition(BasicToken::Type type, size_t token_length, TokenCode operator_token_code = TokenCode::Unspecified);

    void ReadMultilineComment();
    void ReadSingleLineComment();
    void ReadNumericConstant();
    void ReadStringLiteral();
    void ReadStringLiteralVerbatim();
    void ReadOperator(const char* operator_pos);
    void ReadText();

private:
    const char* m_buffer;
    const LogicSettings& m_logicSettings;
    std::vector<BasicToken>& m_basicTokens;

    const char* m_currentBufferPosition;
    const char* m_bufferEndPosition;
    size_t m_lineNumber;
    size_t m_positionInLine;
};


SourceBufferTokenizer::SourceBufferTokenizer(const std::string_view buffer_sv, std::vector<BasicToken>& basic_tokens, const LogicSettings& logic_settings)
    :   m_buffer(buffer_sv.data()),
        m_basicTokens(basic_tokens),
        m_logicSettings(logic_settings),
        m_currentBufferPosition(m_buffer),
        m_bufferEndPosition(m_buffer + buffer_sv.length()),
        m_lineNumber(1),
        m_positionInLine(0)
{
}


char SourceBufferTokenizer::NextChar()
{
    const char ch = *m_currentBufferPosition;

    if( ch != '\0' )
    {
        if( ch == '\n' )
        {
            ++m_lineNumber;
            m_positionInLine = 0;
        }

        else
        {
            ++m_positionInLine;
        }

        ++m_currentBufferPosition;
    }

    return ch;
}


void SourceBufferTokenizer::Tokenize()
{
    bool last_character_was_whitespace = true;
    const char* operator_pos;

    while( AdditionalCharactersExistInBuffer() )
    {
        const char ch = *m_currentBufferPosition;

        // skip over whitespace
        const bool this_character_is_whitespace = std::isspace(ch);

        if( this_character_is_whitespace )
        {
            NextChar();
        }

        // multiline comment start
        else if( CheckFullMatchingText(m_logicSettings.GetMultilineCommentStart(), true) )
        {
            ReadMultilineComment();
        }

        // unbalanced multiline comment end
        else if( CheckFullMatchingText(m_logicSettings.GetMultilineCommentEnd(), true) )
        {
            AddTokenFromCurrentPosition(BasicToken::Type::UnbalancedComment, m_logicSettings.GetMultilineCommentEnd().length());
        }

        // single line comment
        else if( CheckFullMatchingText(m_logicSettings.GetSingleLineComment(), true) )
        {
            ReadSingleLineComment();
        }

        // numeric constant
        else if( std::isdigit(ch) )
        {
            ReadNumericConstant();
        }

        // numeric constant starting with a decimal mark (as long as if follows whitespace)
        else if( ch == '.' && last_character_was_whitespace &&
                 AdditionalCharactersExistInBuffer(2) && std::isdigit(*(m_currentBufferPosition + 1)) )
        {
            ReadNumericConstant();
        }

        // string literal
        else if( is_quotemark(ch) )
        {
            ReadStringLiteral();
        }

        // operator or verbatim string literal
        else if( ( operator_pos = strchr(OperatorCharacters, ch) ) != nullptr )
        {
            if( ch == VerbatimStringLiteralStartCh1 &&
                AdditionalCharactersExistInBuffer(2) && *(m_currentBufferPosition + 1) == VerbatimStringLiteralStartCh2 &&
                m_logicSettings.UseVerbatimStringLiterals() )
            {
                ReadStringLiteralVerbatim();
            }

            else
            {
                ReadOperator(operator_pos);
            }
        }

        // everything else
        else
        {
            ReadText();
        }

        last_character_was_whitespace = this_character_is_whitespace;
    }
}


bool SourceBufferTokenizer::CheckFullMatchingText(const std::string_view text_sv, const bool advance_past_characters)
{
    if( !AdditionalCharactersExistInBuffer(text_sv.length()) ||
        !SO::EqualsNoCase(std::string_view(m_currentBufferPosition, text_sv.length()), text_sv) )
    {
        return false;
    }

    // if a match, read the characters
    if( advance_past_characters )
    {
        for( size_t i = 0; i < text_sv.length(); ++i )
            NextChar();
    }

    return true;
}


void SourceBufferTokenizer::AddTokenFromCurrentPosition(const BasicToken::Type type, const size_t token_length, const TokenCode operator_token_code/* = TokenCode::Unspecified*/)
{
    m_basicTokens.emplace_back(BasicToken
    {
        type,
        operator_token_code,
        token_length,
        m_lineNumber,
        m_positionInLine - token_length,
        m_currentBufferPosition - token_length
    });
}


void SourceBufferTokenizer::ReadMultilineComment()
{
    // save the current state
    const char* const current_buffer_position_after_comment = m_currentBufferPosition;
    size_t line_number = m_lineNumber;
    size_t position_in_line_after_comment = m_positionInLine;

    while( AdditionalCharactersExistInBuffer() )
    {
        // a nested start comment
        if( CheckFullMatchingText(m_logicSettings.GetMultilineCommentStart(), true) )
        {
            ReadMultilineComment();
        }

        // the end of this comment
        else if( CheckFullMatchingText(m_logicSettings.GetMultilineCommentEnd(), true) )
        {
            return;
        }

        else
        {
            NextChar();
        }
    }

    // if here, then there was no end comment
    const size_t multiline_comment_start_length = m_logicSettings.GetMultilineCommentStart().length();

    m_basicTokens.emplace_back(BasicToken
    {
        BasicToken::Type::UnbalancedComment,
        TokenCode::Unspecified,
        multiline_comment_start_length,
        line_number,
        position_in_line_after_comment - multiline_comment_start_length,
        current_buffer_position_after_comment - multiline_comment_start_length,
    });
}


void SourceBufferTokenizer::ReadSingleLineComment()
{
    // read until the end of the line
    while( AdditionalCharactersExistInBuffer() &&
           !is_crlf(*m_currentBufferPosition) )
    {
        NextChar();
    }
}


void SourceBufferTokenizer::ReadNumericConstant()
{
    bool decimal_mark_read = false;
    size_t numeric_length = 0;

    while( AdditionalCharactersExistInBuffer() )
    {
        const char ch = *m_currentBufferPosition;

        if( !std::isdigit(ch) )
        {
            if( !decimal_mark_read && ch == '.' )
            {
                decimal_mark_read = true;
            }

            else
            {
                break;
            }
        }

        NextChar();
        ++numeric_length;
    }

    AddTokenFromCurrentPosition(BasicToken::Type::NumericConstant, numeric_length, TokenCode::TOKCTE);
}


void SourceBufferTokenizer::ReadStringLiteral()
{
    const char quotemark = NextChar();
    const char string_literal_escape_sequence = m_logicSettings.EscapeStringLiterals() ? '\\' : 0;
    bool last_character_was_an_escape = false;
    size_t literal_length = 1;

    // copy over the characters up to the end quote or the newline, allowing for escaped characters
    while( AdditionalCharactersExistInBuffer() )
    {
        const char ch = *m_currentBufferPosition;

        if( is_crlf(ch) )
            break;

        NextChar();
        ++literal_length;

        if( last_character_was_an_escape )
        {
            last_character_was_an_escape = false;
        }

        else if( ch == string_literal_escape_sequence )
        {
            last_character_was_an_escape = true;
        }

        else if( ch == quotemark )
        {
            break;
        }
    }

    AddTokenFromCurrentPosition(BasicToken::Type::StringLiteral, literal_length, TokenCode::TOKSCTE);
}


void SourceBufferTokenizer::SourceBufferTokenizer::ReadStringLiteralVerbatim()
{
#ifdef _DEBUG
    ASSERT(NextChar() == VerbatimStringLiteralStartCh1);
    ASSERT(NextChar() == VerbatimStringLiteralStartCh2);
#else
    NextChar();
    NextChar();
#endif

    size_t literal_length = 2;

    // copy over the characters up to the end quote or the newline, allowing for escaped characters
    while( AdditionalCharactersExistInBuffer() )
    {
        const char ch = *m_currentBufferPosition;

        if( is_crlf(ch) )
            break;

        NextChar();
        ++literal_length;

        if( ch == '"' )
        {
            // m_currentBufferPosition points to the character after ch
            if( *m_currentBufferPosition != '"' )
                break;

            // advance past the "" escape
            NextChar();
            ++literal_length;
        }
    }

    AddTokenFromCurrentPosition(BasicToken::Type::StringLiteral, literal_length, TokenCode::TOKSCTE);
}


void SourceBufferTokenizer::ReadOperator(const char* const operator_pos)
{
    const size_t operator_index = operator_pos - OperatorCharacters;
    const char ch = *m_currentBufferPosition;

    BasicToken::Type type = BasicToken::Type::Operator;
    TokenCode operator_token_code = TokenCode::Unspecified;
    size_t operator_length = 1;

    // single character operators
    if( operator_index < _countof(SingleCharacterOperatorTokenCodes) )
    {
        operator_token_code = SingleCharacterOperatorTokenCodes[operator_index];
    }

    // the dollar sign
    else if( ch == '$' )
    {
        type = BasicToken::Type::DollarSign;
    }

    // multiple character operators
    else
    {
        const char second_ch = AdditionalCharactersExistInBuffer(2) ? *(m_currentBufferPosition + 1) : 0;

        if( ch == '-' )
        {
            if( second_ch == '>' )
            {
                ++operator_length;
                operator_token_code = TokenCode::TOKARROW; // ->
            }

            else
            {
                operator_token_code = TokenCode::TOKMINOP; // -
            }
        }

        else if( ch == ':' )
        {
            if( second_ch == ':' )
            {
                ++operator_length;
                operator_token_code = TokenCode::TOKDOUBLECOLON; // ::
            }

            else if( second_ch == '=' )
            {
                ++operator_length;
                operator_token_code = TokenCode::TOKNAMEDARGOP; // :=
            }

            else
            {
                operator_token_code = TokenCode::TOKCOLON; // :
            }
        }

        else if( ch == '<' )
        {
            if( second_ch == '=' )
            {
                ++operator_length;

                const char third_ch = AdditionalCharactersExistInBuffer(3) ? *(m_currentBufferPosition + 2) : 0;

                if( third_ch == '>' )
                {
                    ++operator_length;
                    operator_token_code = TokenCode::TOKEQUOP; // <=>
                }

                else
                {
                    operator_token_code = TokenCode::TOKLEOP; // <=
                }
            }

            else if( second_ch == '>' )
            {
                ++operator_length;
                operator_token_code = TokenCode::TOKNEOP; // <>
            }

            else
            {
                operator_token_code = TokenCode::TOKLTOP; // <
            }
        }

        else if( ch == '>' )
        {
            if( second_ch == '=' )
            {
                ++operator_length;
                operator_token_code = TokenCode::TOKGEOP; // >=
            }

            else
            {
                operator_token_code = TokenCode::TOKGTOP; // >
            }
        }

        else
        {
            ASSERT(ch == '=');

            if( second_ch == '>' )
            {
                ++operator_length;
                operator_token_code = TokenCode::TOKIMPLOP; // =>
            }

            else
            {
                operator_token_code = TokenCode::TOKEQOP; // =
            }
        }
    }

    ASSERT(type == BasicToken::Type::DollarSign || operator_token_code != TokenCode::Unspecified);

    // read the characters
    for( size_t i = 0; i < operator_length; ++i )
        NextChar();

    AddTokenFromCurrentPosition(type, operator_length, operator_token_code);
}


void SourceBufferTokenizer::ReadText()
{
    // read all characters until a whitespace character, an operator, or a comment
    size_t text_length = 0;

    while( AdditionalCharactersExistInBuffer() )
    {
        const char ch = *m_currentBufferPosition;

        // break on whitespace
        if( std::isspace(ch) )
            break;

        // break on an operator, though for speed, before checking if the character is an operator,
        // check if it is a typically encounted character
        if( !is_alnum(ch) && strchr(OperatorCharacters, ch) != nullptr )
            break;

        // break on a comment
        if( CheckFullMatchingText(m_logicSettings.GetMultilineCommentStart(), false) ||
            CheckFullMatchingText(m_logicSettings.GetSingleLineComment(), false) )
        {
            break;
        }

        NextChar();
        ++text_length;
    }

    AddTokenFromCurrentPosition(BasicToken::Type::Text, text_length);
}



// --------------------------------------------------------------------------
// SourceBuffer
// --------------------------------------------------------------------------

SourceBuffer::SourceBuffer(SharableString buffer)
    :   m_buffer(std::move(buffer))
{
}


std::vector<BasicToken> SourceBuffer::Tokenize(const SharableString& buffer, const LogicSettings& logic_settings)
{
    std::vector<BasicToken> basic_tokens;

    SourceBufferTokenizer source_buffer_tokenizer(*buffer, basic_tokens, logic_settings);
    source_buffer_tokenizer.Tokenize();

    return basic_tokens;
}


const std::vector<BasicToken>& SourceBuffer::Tokenize(const LogicSettings& logic_settings)
{
    if( !m_basicTokens.has_value() )
    {
        m_basicTokens = Tokenize(m_buffer, logic_settings);

        // adjust line numbers if necessary
        if( m_lineAdjuster != nullptr )
        {
            for( BasicToken& basic_token : *m_basicTokens )
                basic_token.line_number = m_lineAdjuster->GetLineNumber(basic_token.line_number);
        }
    }

    return *m_basicTokens;
}


const std::vector<BasicToken>& SourceBuffer::GetTokens() const
{
    ASSERT(m_basicTokens.has_value());
    return *m_basicTokens;
}


size_t SourceBuffer::GetPositionInBuffer(const BasicToken& basic_token) const
{
    ASSERT(basic_token.token_text >= m_buffer->data() &&
           basic_token.token_text <= ( m_buffer->data() + m_buffer->length() ));

    return ( basic_token.token_text - m_buffer->data() );
}


void SourceBuffer::RemoveTokensAfterText(const size_t start_position, const TokenCode token_code,
                                         const cs::cref_optional<std::string> end_text/* = std::nullopt*/)
{
    ASSERT(m_basicTokens.has_value());

    auto basic_tokens_itr = m_basicTokens->cbegin();
    const auto& basic_tokens_end = m_basicTokens->cend();

    // find the start position
    for( ; basic_tokens_itr != basic_tokens_end && GetPositionInBuffer(*basic_tokens_itr) < start_position; ++basic_tokens_itr )
    {
    }

    // process the tokens that exist after the start position
    for( ; basic_tokens_itr != basic_tokens_end; ++basic_tokens_itr )
    {
        if( basic_tokens_itr->token_code == token_code )
        {
            if( !end_text.has_value() || SO::EqualsNoCase(*end_text, basic_tokens_itr->GetSV()) )
            {
                m_basicTokens->erase(basic_tokens_itr + 1, basic_tokens_end);
                return;
            }
        }
    }
}
