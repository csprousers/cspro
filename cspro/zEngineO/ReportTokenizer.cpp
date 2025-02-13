#include "stdafx.h"
#include "ReportTokenizer.h"
#include <zLogicO/LogicScanner.h>


bool ReportTokenizer::Tokenize(wstring_view report_text_sv, const LogicSettings& logic_settings)
{
    size_t line_number = 1;

    // the logic scanner is used to prevent switching sections while in comments or string literals
    Logic::LogicScanner logic_scanner(logic_settings);

    // analyze the report character by character and break it up into tokens
    m_reportTokens.emplace_back(ReportToken { ReportToken::Type::ReportText, line_number });

    const auto& report_text_end = report_text_sv.cend();

    for( auto report_text_itr = report_text_sv.cbegin(); report_text_itr < report_text_end; ++report_text_itr )
    {
        auto get_future_ch = [&](size_t index) -> TCHAR
        {
            auto future_ch_pos = report_text_itr + index;
            return ( future_ch_pos < report_text_end ) ? *future_ch_pos : 0;
        };

        const TCHAR ch = *report_text_itr;
        const TCHAR next_ch = get_future_ch(1);

        if( ( ch == '\r' && next_ch != '\n' ) || ch == '\n' )
            ++line_number;

        auto ch_starts_double_tilde = [&]() { return ( ch == '~' && next_ch == '~' && get_future_ch(2) != '~' ); };
        auto ch_starts_triple_tilde = [&]() { return ( ch == '~' && next_ch == '~' && get_future_ch(2) == '~' ); };
        auto ch_starts_end_logic    = [&]() { return ( ch == '?' && next_ch == '>' ); };

        ReportToken& current_token = m_reportTokens.back();

        bool section_changed = false;

        auto change_section = [&](ReportToken::Type new_report_token_type, size_t extra_characters_processed)
        {
            m_reportTokens.emplace_back(ReportToken { new_report_token_type, line_number });
            report_text_itr += extra_characters_processed;
            section_changed = true;
        };

        // if in report text...
        // --------------------------------
        if( current_token.type == ReportToken::Type::ReportText )
        {
            ASSERT(!logic_scanner.InSpecialSection());

            // switching to CSPro logic
            if( ch == '<' && next_ch == '?' )
            {
                change_section(ReportToken::Type::Logic, 1);
            }

            // switching to a ~~ fill
            else if( ch_starts_double_tilde() )
            {
                change_section(ReportToken::Type::DoubleTilde, 1);
            }

            // switching to a ~~~ fill
            else if( ch_starts_triple_tilde() )
            {
                change_section(ReportToken::Type::TripleTilde, 2);
            }

            // unbalanced <? ?> escapes
            else if( ch_starts_end_logic() )
            {
                OnErrorUnbalancedEscapes(line_number);
                return false;
            }
        }

        // if in a fill (~~)
        // --------------------------------
        else if( current_token.type == ReportToken::Type::DoubleTilde )
        {
            // switching back to the report
            if( !logic_scanner.InSpecialSection() && ch_starts_double_tilde() )
                change_section(ReportToken::Type::ReportText, 1);
        }

        // if in a fill (~~~)
        // --------------------------------
        else if( current_token.type == ReportToken::Type::TripleTilde )
        {
            // switching back to the report
            if( !logic_scanner.InSpecialSection() && ch_starts_triple_tilde() )
                change_section(ReportToken::Type::ReportText, 2);
        }

        // if in logic
        // --------------------------------
        else if( current_token.type == ReportToken::Type::Logic )
        {
            // switching back to the report
            if( !logic_scanner.InSpecialSection() && ch_starts_end_logic() )
                change_section(ReportToken::Type::ReportText, 1);
        }


        // if the section did not change...
        if( !section_changed )
        {
            // ...add the character to the current token
            current_token.text.push_back(ch);

            // ...and when in a fill or logic, keep track of special sections
            if( current_token.type != ReportToken::Type::ReportText )
                logic_scanner.ProcessCharacter(ch, next_ch);
        }
    }


    // the report cannot end in logic or a fill
    if( m_reportTokens.back().type != ReportToken::Type::ReportText )
    {
        OnErrorTokenNotEnded(m_reportTokens.back());
        return false;
    }

    return true;
}
