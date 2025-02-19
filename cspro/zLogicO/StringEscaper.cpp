#include "stdafx.h"
#include "StringEscaper.h"
#include <zToolsO/Encoders.h>

using namespace Logic;


StringEscaper::StringEscaper(const bool escape_string_literals)
    :   m_escapeStringLiterals(escape_string_literals)
{
}


std::string StringEscaper::EscapeString(std::string text, const bool use_verbatim_string_literals/* = false*/) const
{
    ASSERT(m_escapeStringLiterals || !use_verbatim_string_literals);

    return use_verbatim_string_literals ? EscapeStringUsingVerbatimStringLiterals(text) :
           m_escapeStringLiterals       ? Encoders::ToLogicString(std::move(text)) :
                                          EscapeStringForOldLogic(std::move(text));
}


std::string StringEscaper::EscapeStringUsingVerbatimStringLiterals(std::string text) const
{
    if( text.empty() )
        return "@\"\"";

    // the only thing that should be escaped in a verbatim string literal is double quotes, but to make this
    // more robust, we will also handle non-printable characters (by escaping them as non-verbatim string literals)
    size_t first_double_quote_pos = SIZE_MAX;
    size_t first_control_character_pos = SIZE_MAX;

    const char* const text_start = text.c_str();
    const char* text_itr = text_start;

    for( size_t i = 0; ; ++i, ++text_itr )
    {
        const unsigned char ch = static_cast<unsigned char>(*text_itr);

        if( ch <= Encoders::LastControlCharacter )
        {
            if( ch == '\0' )
                break;

            if( first_control_character_pos == SIZE_MAX )
                first_control_character_pos = i;

            if( first_double_quote_pos != SIZE_MAX )
                break;
        }

        else if( *text_itr == '"' )
        {
            if( first_double_quote_pos == SIZE_MAX )
                first_double_quote_pos = i;

            if( first_control_character_pos != SIZE_MAX )
                break;
        }
    }

    auto surround_by_at_and_quotes = [](const char* const start_text_ptr, const size_t text_length, const size_t double_quote_pos)
    {
        constexpr std::string_view Prefix_sv = "@\"";

        std::string verbatim_literal = SO::Concatenate(Prefix_sv, std::string_view(start_text_ptr, text_length));

        if( double_quote_pos != SIZE_MAX )
            SO::Replace(verbatim_literal, "\"", "\"\"", double_quote_pos + Prefix_sv.length());

        verbatim_literal.push_back('"');

        return verbatim_literal;
    };

    // without control characters, possibly escape double quotes and then surround by @"..."
    if( first_control_character_pos == SIZE_MAX )
    {
        return surround_by_at_and_quotes(text_start, text.length(), first_double_quote_pos);
    }

    // when control characters are present, mix verbatim string literals with normal string literals
    std::string logic_string = ( first_control_character_pos > 0 ) ? surround_by_at_and_quotes(text_start, first_control_character_pos, first_double_quote_pos) :
                                                                     std::string();

    const char* block_start_pos = text_start + first_control_character_pos;
    bool in_control_character_block = true;

    for( text_itr = block_start_pos + 1; true; ++text_itr )
    {
        const bool ch_is_control_character = ( static_cast<unsigned char>(*text_itr) <= Encoders::LastControlCharacter );
        const bool end_of_string = ( ch_is_control_character && *text_itr == 0 );

        if( !end_of_string && in_control_character_block == ch_is_control_character )
            continue;

        // end the block
        const size_t block_length = text_itr - block_start_pos;
        ASSERT(block_length > 0);

        // separate string literals with single space
        if( !logic_string.empty() )
            logic_string.push_back(' ');

        if( in_control_character_block )
        {
            logic_string.append(Encoders::ToLogicString(std::string(block_start_pos, block_length)));
        }

        else
        {
            logic_string.append(surround_by_at_and_quotes(block_start_pos, block_length, 0));
        }

        if( end_of_string )
            return logic_string;

        block_start_pos = text_itr;
        in_control_character_block = ch_is_control_character;
    }
}


std::string StringEscaper::EscapeStringWithSplitNewlines(std::string text, const bool use_verbatim_string_literals/* = false*/) const
{
    // text should only come in with \n newline characters
    ASSERT(text.find('\r') == std::string::npos);

    if( !m_escapeStringLiterals || !SO::ContainsNewlineCharacter(text) )
    {
        return EscapeString(std::move(text), use_verbatim_string_literals);
    }

    else
    {
        std::string logic;
        std::vector<std::string> lines = SO::SplitString(text, '\n', false, true);

        for( size_t i = 0; i < lines.size(); ++i )
        {
            std::string line = std::move(lines[i]);

            if( ( i + 1 ) < lines.size() )
                line.push_back('\n');

            SO::AppendWithSeparator(logic, EscapeString(std::move(line), use_verbatim_string_literals), '\n');
        }

        return logic;
    }
}


std::string StringEscaper::EscapeStringForOldLogic(std::string text) const
{
    // using old logic settings, remove any escape sequences that would not be properly handled
    constexpr std::string_view escape_representations_allowed_sv = "\'\"\\";
    static_assert(std::string_view(EncoderEscapes::Representations).substr(0, escape_representations_allowed_sv.length()) == escape_representations_allowed_sv);
    constexpr const char* const escape_representations_to_use = EncoderEscapes::Representations + escape_representations_allowed_sv.length();

    for( auto text_itr = text.begin(); text_itr != text.end(); )
    {
        const char ch = *text_itr;

        if( strchr(escape_representations_to_use, ch) != nullptr )
        {
            text_itr = text.erase(text_itr);
        }

        else
        {
            ++text_itr;
        }
    }

    // if double or single quotes are unused, surround the string in that quotemark
    for( const char quote_ch : { '"', '\'' } )
    {
        const size_t quote_pos = text.find(quote_ch);

        if( quote_pos == std::string::npos )
            return quote_ch + text + quote_ch;
    }

    // if both quote characters are used, take advantage of the fact that string literals
    // tokens are automatically concatenated to generate a string that uses both quotemarks
    std::optional<char> quotemark_in_use;
    size_t start_quotemark_pos = 0;

    auto end_current_quotemark = [&](const size_t end_quotemark_pos)
    {
        ASSERT(quotemark_in_use.has_value());
        text.insert(text.begin() + end_quotemark_pos, *quotemark_in_use);
        text.insert(text.begin() + start_quotemark_pos, *quotemark_in_use);
    };

    for( size_t i = 0; i < text.length(); ++i )
    {
        const char ch = text[i];
        const bool single_quote = ( ch == '\'' );

        // if this is not a quote, or is the opposite of the current quotemark, allow the character
        if( ( !single_quote && ch != '"' ) || ( quotemark_in_use.has_value() && ch != *quotemark_in_use ) )
            continue;

        if( quotemark_in_use.has_value() )
        {
            end_current_quotemark(i);
            i += 2;

            // insert a space to separate the quoted strings
            text.insert(text.begin() + i, ' ');
            ++i;

            start_quotemark_pos = i;
        }

        quotemark_in_use = single_quote ? '"' : '\'';
    }

    end_current_quotemark(text.length());

    return text;
}
