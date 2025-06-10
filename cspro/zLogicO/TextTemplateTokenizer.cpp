#include "stdafx.h"
#include "TextTemplateTokenizer.h"
#include "BaseCompiler.h"
#include "LogicScanner.h"


// --------------------------------------------------------------------------
// TextTemplateTokenizer
// --------------------------------------------------------------------------

TextTemplateTokenizer::TextTemplateTokenizer(const bool allow_logic_escapes)
    :   m_allowLogicEscapes(allow_logic_escapes)
{
}


bool TextTemplateTokenizer::Tokenize(const std::string_view text_template_sv, const LogicSettings& logic_settings)
{
    size_t line_number = 1;

    // the logic scanner is used to prevent switching sections while in comments or string literals
    Logic::LogicScanner logic_scanner(logic_settings);

    // analyze the text template character by character and break it up into tokens
    m_tokens.emplace_back(TextTemplateToken { TextTemplateToken::Type::DirectText, line_number });

    const auto& text_end = text_template_sv.cend();

    for( auto text_itr = text_template_sv.cbegin(); text_itr < text_end; ++text_itr )
    {
        auto get_future_ch = [&](const size_t index) -> char
        {
            const auto future_ch_pos = text_itr + index;
            return ( future_ch_pos < text_end ) ? *future_ch_pos : 0;
        };

        const char ch = *text_itr;
        const char next_ch = get_future_ch(1);

        if( ( ch == '\r' && next_ch != '\n' ) || ch == '\n' )
            ++line_number;

        auto ch_starts_double_tilde = [&]() { return ( ch == '~' && next_ch == '~' && get_future_ch(2) != '~' ); };
        auto ch_starts_triple_tilde = [&]() { return ( ch == '~' && next_ch == '~' && get_future_ch(2) == '~' ); };
        auto ch_starts_end_logic    = [&]() { ASSERT(m_allowLogicEscapes); return ( ch == '?' && next_ch == '>' ); };

        TextTemplateToken& current_token = m_tokens.back();

        bool section_changed = false;

        auto change_section = [&](const TextTemplateToken::Type new_token_type, const size_t extra_characters_processed)
        {
            m_tokens.emplace_back(TextTemplateToken { new_token_type, line_number });
            text_itr += extra_characters_processed;
            section_changed = true;
        };

        // if in direct text...
        // --------------------------------
        if( current_token.type == TextTemplateToken::Type::DirectText )
        {
            ASSERT(!logic_scanner.InSpecialSection());

            // switching to CSPro logic
            if( m_allowLogicEscapes && ch == '<' && next_ch == '?' )
            {
                change_section(TextTemplateToken::Type::Logic, 1);
            }

            // switching to a ~~ fill
            else if( ch_starts_double_tilde() )
            {
                change_section(TextTemplateToken::Type::DoubleTilde, 1);
            }

            // switching to a ~~~ fill
            else if( ch_starts_triple_tilde() )
            {
                change_section(TextTemplateToken::Type::TripleTilde, 2);
            }

            // unbalanced <? ?> escapes
            else if( m_allowLogicEscapes && ch_starts_end_logic() )
            {
                OnErrorUnbalancedEscapes(line_number);
                return false;
            }
        }

        // if in a fill (~~)
        // --------------------------------
        else if( current_token.type == TextTemplateToken::Type::DoubleTilde )
        {
            // switching back to the text template's direct text
            if( !logic_scanner.InSpecialSection() && ch_starts_double_tilde() )
                change_section(TextTemplateToken::Type::DirectText, 1);
        }

        // if in a fill (~~~)
        // --------------------------------
        else if( current_token.type == TextTemplateToken::Type::TripleTilde )
        {
            // switching back to the text template's direct text
            if( !logic_scanner.InSpecialSection() && ch_starts_triple_tilde() )
                change_section(TextTemplateToken::Type::DirectText, 2);
        }

        // if in logic
        // --------------------------------
        else if( current_token.type == TextTemplateToken::Type::Logic )
        {
            ASSERT(m_allowLogicEscapes);

            // switching back to the text template's direct text
            if( !logic_scanner.InSpecialSection() && ch_starts_end_logic() )
                change_section(TextTemplateToken::Type::DirectText, 1);
        }


        // if the section did not change...
        if( !section_changed )
        {
            // ...add the character to the current token
            current_token.text.push_back(ch);

            // ...and when in a fill or logic, keep track of special sections
            if( current_token.type != TextTemplateToken::Type::DirectText )
                logic_scanner.ProcessCharacter(ch, next_ch);
        }
    }


    // the text template cannot end in logic or a fill
    if( m_tokens.back().type != TextTemplateToken::Type::DirectText )
    {
        OnErrorTokenNotEnded(m_tokens.back());
        return false;
    }

    return true;
}


bool TextTemplateTokenizer::IsOnlyDirectTextUsed() const
{
    ASSERT(!m_tokens.empty() && m_tokens.front().type == TextTemplateToken::Type::DirectText);

    return ( m_tokens.size() == 1 );
}



// --------------------------------------------------------------------------
// ErrorReportingTextTemplateTokenizer
// --------------------------------------------------------------------------

ErrorReportingTextTemplateTokenizer::ErrorReportingTextTemplateTokenizer(Logic::BasicTokenCompiler& logic_compiler, const bool allow_logic_escapes)
        :   TextTemplateTokenizer(allow_logic_escapes),
            m_compiler(logic_compiler)
{
}


void ErrorReportingTextTemplateTokenizer::OnErrorUnbalancedEscapes(const size_t line_number)
{
    m_compiler.ReportError(MGF::TextTemplate_unbalanced_escapes_48101, static_cast<int>(line_number));
}


void ErrorReportingTextTemplateTokenizer::OnErrorTokenNotEnded(const TextTemplateToken& token)
{
    m_compiler.ReportError(MGF::TextTemplate_end_reached_while_in_logic_or_fill_48102,
                           ( token.type == TextTemplateToken::Type::Logic ) ? "logic" : "a fill",
                           static_cast<int>(token.section_line_number_start));
}
