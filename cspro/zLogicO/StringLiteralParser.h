#pragma once

#include <zToolsO/Encoders.h>


namespace Logic
{
    class StringLiteralParser
    {
    public:
        // this function indicates whether the text begins as a string literal (or verbatim string literal)
        template<typename CF>
        static bool IsStringLiteralStart(TCHAR first_ch, CF get_second_ch_callback_function);

        static bool IsStringLiteralStart(wstring_view text_sv);

        // this function processes a string literal (or verbatim string literal);
        // if EndQuotemarkMayExistBeforeTextEnd is true, the function returns the index of the character following the end quotemark
        template<bool UseLogicSettings = true,
                 bool IfNotUseLogicSettingsIsV8Plus = true,
                 bool EndQuotemarkMayExistBeforeTextEnd = false,
                 typename Compiler>
        static auto Parse(Compiler& compiler, std::wstring& parsed_text, wstring_view text_sv);
    };
}



template<typename CF>
bool Logic::StringLiteralParser::IsStringLiteralStart(TCHAR first_ch, CF get_second_ch_callback_function)
{
    return ( is_quotemark(first_ch) ||
             first_ch == VerbatimStringLiteralStartCh1 && get_second_ch_callback_function() == VerbatimStringLiteralStartCh2 );

}


inline bool Logic::StringLiteralParser::IsStringLiteralStart(wstring_view text_sv)
{
    return ( !text_sv.empty() &&
             IsStringLiteralStart(text_sv.front(), [&]() { return ( text_sv.length() > 1 ) ? text_sv[1] : 0; }) );
}


template<bool UseLogicSettings/* = true*/,
         bool IfNotUseLogicSettingsIsV8Plus/* = true*/,
         bool EndQuotemarkMayExistBeforeTextEnd/* = false*/,
         typename Compiler>
auto Logic::StringLiteralParser::Parse(Compiler& compiler, std::wstring& parsed_text, wstring_view text_sv)
{
    const TCHAR* text_itr = text_sv.data();
    const TCHAR* const text_last_character_ptr = text_itr + text_sv.length() - 1;
    ASSERT(text_itr <= text_last_character_ptr);

    constexpr TCHAR VerbatimStringLiteralEscapeSequence = VerbatimStringLiteralStartCh2;

    bool last_character_was_an_escape = false;
    TCHAR quotemark;
    TCHAR string_literal_escape_sequence;

    // for verbatim string literals, the quotemark (double quote) is the second character
    if( *text_itr == VerbatimStringLiteralStartCh1 )
    {
        if constexpr(UseLogicSettings)
        {
            ASSERT(compiler.GetLogicSettings().UseVerbatimStringLiterals());
        }

        else
        {
            ASSERT(IfNotUseLogicSettingsIsV8Plus);
        }

        ASSERT(text_itr[1] == VerbatimStringLiteralStartCh2);
        quotemark = VerbatimStringLiteralStartCh2;
        string_literal_escape_sequence = VerbatimStringLiteralEscapeSequence;

        text_itr += 2;
    }

    // for normal string literals, the quotemark is the first character
    else
    {
        quotemark = *text_itr;

        if constexpr(UseLogicSettings)
        {
            string_literal_escape_sequence = compiler.GetLogicSettings().EscapeStringLiterals() ? '\\' : 0;
        }

        else
        {
            string_literal_escape_sequence = IfNotUseLogicSettingsIsV8Plus ? '\\' : 0;
        }

        ++text_itr;
    }

    ASSERT(is_quotemark(quotemark));

    // unbalanced string literal (check 1)
    if( text_itr > text_last_character_ptr )
        compiler.IssueError(92181, quotemark);

    // iterate from the first non-quotemark character to the second-from-last character
    for( ; text_itr < text_last_character_ptr; ++text_itr )
    {
        const TCHAR ch = *text_itr;

        if( last_character_was_an_escape )
        {
            if( ch == VerbatimStringLiteralEscapeSequence )
            {
                ASSERT(ch == VerbatimStringLiteralStartCh2);
                parsed_text.push_back(VerbatimStringLiteralStartCh2);
            }

            else
            {
                // if the text hasn't been fully parsed, this may be the end quote of a verbatim string literal
                // this checks if this is a verbatim string literal: string_literal_escape_sequence == VerbatimStringLiteralEscapeSequence
                if constexpr(EndQuotemarkMayExistBeforeTextEnd)
                {
                    if( string_literal_escape_sequence == VerbatimStringLiteralEscapeSequence && ch == VerbatimStringLiteralEscapeSequence )
                        break;
                }

                else
                {
                    ASSERT(string_literal_escape_sequence != VerbatimStringLiteralEscapeSequence);
                }

                const TCHAR escaped_representation = Encoders::GetEscapedRepresentation(ch);

                // invalid escape sequence
                if( escaped_representation == 0 )
                    compiler.IssueError(92182, string_literal_escape_sequence, ch);

                parsed_text.push_back(escaped_representation);
            }

            last_character_was_an_escape = false;
        }

        else if( ch == string_literal_escape_sequence )
        {
            last_character_was_an_escape = true;
        }

        else
        {
            // if the text hasn't been fully parsed, this may the end quote of a normal string literal
            if constexpr(EndQuotemarkMayExistBeforeTextEnd)
            {
                if( ch == quotemark )
                    break;
            }

            parsed_text.push_back(ch);
        }
    }

    ASSERT(EndQuotemarkMayExistBeforeTextEnd ? ( text_itr <= text_last_character_ptr ) :
                                               ( text_itr == text_last_character_ptr ));

    // unbalanced string literal (check 2)
    if( last_character_was_an_escape || *text_itr != quotemark )
        compiler.IssueError(92181, quotemark);

    // return the location following the end quotemark
    if constexpr(EndQuotemarkMayExistBeforeTextEnd)
        return text_itr + 1 - text_sv.data();
}
