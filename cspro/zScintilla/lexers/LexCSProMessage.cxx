#include "LexCSPro.h"
#include <zToolsO/EscapesAndLogicOperators.h>
#include <memory>
#include <optional>

using namespace Scintilla;
using namespace Lexilla;


class LexerCSProMessage : public DefaultLexer
{
public:
    LexerCSProMessage(int language);

    const char* SCI_METHOD PropertyGet(const char* key) override;
    int SCI_METHOD LineEndTypesSupported() override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;

private:
    size_t GetMultilineCommentState(const std::vector<bool>* multiline_comment_state);

private:
    constexpr static size_t MultilineCommentsStateIndex = 1;
    constexpr static bool MultilineCommentsNewExpected = true;
    constexpr static bool MultilineCommentsOldExpected = false;

    bool m_isV8_0;
    std::vector<std::vector<bool>> m_multilineCommentStates;
};


LexerCSProMessage::LexerCSProMessage(int language)
    :   DefaultLexer(LexCSPro::Name(language), language),
        m_isV8_0(LexCSPro::IsV8_0(language))
{
    m_multilineCommentStates.emplace_back();
    assert(m_multilineCommentStates.size() == MultilineCommentsStateIndex);
}


const char* LexerCSProMessage::PropertyGet(const char* /*key*/)
{
    assert(false);
    return nullptr;
}


int LexerCSProMessage::LineEndTypesSupported()
{
    return SC_LINE_END_TYPE_UNICODE;
}


size_t LexerCSProMessage::GetMultilineCommentState(const std::vector<bool>* multiline_comment_state)
{
    if( multiline_comment_state == nullptr )
        return 0;

    const auto& state_lookup = std::find_if(m_multilineCommentStates.cbegin(), m_multilineCommentStates.cend(),
                                            [&](const std::vector<bool>& state) { return ( *multiline_comment_state == state ); });

    // reuse an existing entry that matches the current state...
    if( state_lookup != m_multilineCommentStates.cend() )
    {
        return std::distance(m_multilineCommentStates.cbegin(), state_lookup);
    }

    // ...or add a new one
    else
    {
        m_multilineCommentStates.emplace_back(*multiline_comment_state);
        return m_multilineCommentStates.size() - 1;
    }
}


void LexerCSProMessage::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess)
{
    Accessor styler(pAccess, nullptr);
    StyleContext sc(startPos, length, initStyle, styler);

    // get the state of multiline comments on this line
    const size_t initial_line_state = styler.GetLineState(sc.currentLine - 1);
    assert(initial_line_state < m_multilineCommentStates.size());

    std::unique_ptr<std::vector<bool>> multiline_comment_state;
    std::optional<Sci_Position> last_added_multiline_comment_line;

    if( initial_line_state >= MultilineCommentsStateIndex )
    {
        multiline_comment_state = std::make_unique<std::vector<bool>>(m_multilineCommentStates[initial_line_state]);
        assert(!multiline_comment_state->empty());

        sc.ChangeState(SCE_CSPRO_COMMENT);
    }

    enum class ExtendedState 
    {
        InvalidLine,
        MultilineCommentLine,
        LanguageLine,
        ProcessingLanguageName,
        ExpectingLeftParenthesis,
        ExpectingTranslation,
        ProcessingTranslation,
        ExpectingRightParenthesis,        
        ExpectingMessageText,
        ProcessingMessageText,
        ExpectingEndOfLine,
    };

    std::optional<ExtendedState> extended_state;
    int string_quotemark = 0;
    bool string_is_verbatim = false;
    bool move_forward_at_end_of_while_loop = true;

    while( sc.More() )
    {
        // on a new line, update the multiline comment state...
        if( sc.atLineStart )
        {
            styler.SetLineState(sc.currentLine, GetMultilineCommentState(multiline_comment_state.get()));

            sc.SetState(( multiline_comment_state == nullptr ) ? SCE_CSPRO_DEFAULT : 
                                                                 SCE_CSPRO_COMMENT);
            extended_state.reset();

            // ... and read past whitespace at the beginning of the line
            while( isspacechar(sc.ch) && sc.More() )
                sc.Forward();
        }

        // multiline comments can end on the same line where they were started, or at the beginning of a line
        if( sc.state == SCE_CSPRO_COMMENT )
        {
            assert(multiline_comment_state != nullptr && !multiline_comment_state->empty());
            const bool must_end_new_style_comment = ( multiline_comment_state->back() == MultilineCommentsNewExpected );

            if( !extended_state.has_value() || last_added_multiline_comment_line == sc.currentLine )
            {
                if( must_end_new_style_comment ? sc.Match('*', '/') : sc.Match('}') )
                {
                    if( must_end_new_style_comment )
                        sc.Forward();

                    multiline_comment_state->pop_back();                    

                    if( multiline_comment_state->empty() )
                    {
                        multiline_comment_state.reset();

                        sc.ForwardSetState(SCE_CSPRO_DEFAULT);
                        move_forward_at_end_of_while_loop = false;
                        extended_state = ExtendedState::ExpectingEndOfLine;
                    }

                    else if( last_added_multiline_comment_line == sc.currentLine )
                    {
                        sc.Forward();
                        extended_state = ExtendedState::ExpectingEndOfLine;
                    }

                    styler.SetLineState(sc.currentLine, GetMultilineCommentState(multiline_comment_state.get()));
                }
            }
        }

        // if not ending a multiline comment (above), the first non-whitespace character of the line can be:
        // - a start comment: // or /* or {
        // - a number
        // - Language followed by a =
        // - a language name followed by (number/string)
        if( !extended_state.has_value() && ( sc.state == SCE_CSPRO_DEFAULT || sc.state == SCE_CSPRO_COMMENT ) )
        {
            if( sc.Match('/', '/') )
            {
                sc.SetState(SCE_CSPRO_COMMENTLINE);
            }

            else if( const bool new_style_comment = sc.Match('/', '*'); new_style_comment || sc.Match('{') )
            {
                sc.SetState(SCE_CSPRO_COMMENT);

                if( new_style_comment )
                    sc.Forward();

                if( multiline_comment_state == nullptr )
                    multiline_comment_state = std::make_unique<std::vector<bool>>();

                multiline_comment_state->emplace_back(new_style_comment ? MultilineCommentsNewExpected : MultilineCommentsOldExpected);
                last_added_multiline_comment_line = sc.currentLine;

                extended_state = ExtendedState::MultilineCommentLine;

                styler.SetLineState(sc.currentLine, GetMultilineCommentState(multiline_comment_state.get()));
            }

            else if( sc.state == SCE_CSPRO_COMMENT )
            {
                extended_state = ExtendedState::MultilineCommentLine;
            }
            
            else if( IsADigit(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_NUMBER);
            }

            else if( IsAlpha(sc.ch) )
            {
                constexpr const char* LanguageChangeAction = "Language";
                char language_check[std::string_view(LanguageChangeAction).length() + 1];

                sc.GetCurrent(language_check, sizeof(language_check));

                // ...a Language= directive will all be colored in the default color
                if( _stricmp(LanguageChangeAction, language_check) == 0 )
                {
                    extended_state = ExtendedState::LanguageLine;
                }

                // otherwide this should be a name followed by a (
                else
                {
                    extended_state = ExtendedState::ExpectingLeftParenthesis;
                }
            }

            else
            {
                extended_state = ExtendedState::InvalidLine;
            }
        }

        // if in a string literal, check for escapes or for the end quotemark
        else if( sc.state == SCE_CSPRO_STRING )
        {
            assert(extended_state == ExtendedState::ProcessingTranslation ||
                   extended_state == ExtendedState::ProcessingMessageText);

            // message text specified without using a string literal is not escaped
            if( string_quotemark != 0 )
            {
                // verbatim string literal escapes are double quoted: ""
                // string literals are escaped with a backslash: \[char]
                if( string_is_verbatim ? sc.Match('"', '"') : 
                                         sc.Match('\\') )
                {
                    sc.SetState(SCE_CSPRO_STRING_ESCAPE);
                }

                // ending a string literal
                else if( sc.Match(string_quotemark) )
                {
                    sc.ForwardSetState(SCE_CSPRO_DEFAULT);
                    move_forward_at_end_of_while_loop = false;

                    extended_state = ( extended_state == ExtendedState::ProcessingTranslation ) ? ExtendedState::ExpectingRightParenthesis :
                                                                                                  ExtendedState::ExpectingEndOfLine;
                }
            }
        }

        // handle string escapes
        else if( sc.state == SCE_CSPRO_STRING_ESCAPE )
        {
            assert(sc.chPrev == ( string_is_verbatim ? '"' : '\\' ));

            // if an escape (for a normal string literal) is not valid, color it as a normal string
            if( !string_is_verbatim && wcschr(Encoders::EscapeSequences, sc.ch) == nullptr )
                sc.ChangeState(SCE_CSPRO_STRING);

            sc.ForwardSetState(SCE_CSPRO_STRING);
            move_forward_at_end_of_while_loop = false;
        }

        // process the language name prior to the (
        else if( extended_state == ExtendedState::ExpectingLeftParenthesis )
        {
            if( sc.Match('(') )
            {
                extended_state = ExtendedState::ExpectingTranslation;
            }

            else if( isspacechar(sc.ch) )
            {
                if( !isspacechar(sc.chNext) && sc.chNext != '(' )
                    extended_state = ExtendedState::InvalidLine;
            }

            else if( !IsCSProWordChar(sc.ch) )
            {
                extended_state = ExtendedState::InvalidLine;
            }
        }

        // process a translation number
        else if( extended_state == ExtendedState::ExpectingTranslation && IsADigit(sc.ch) )
        {
            sc.SetState(SCE_CSPRO_NUMBER);
            extended_state = ExtendedState::ProcessingTranslation;
        }

        // process a translation string literal, or the message text
        else if( extended_state == ExtendedState::ExpectingTranslation ||
                 extended_state == ExtendedState::ExpectingMessageText )
        {
            if( is_quotemark(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_STRING);
                extended_state = ( extended_state == ExtendedState::ExpectingTranslation ) ? ExtendedState::ProcessingTranslation :
                                                                                             ExtendedState::ProcessingMessageText;

                string_is_verbatim = false;
                string_quotemark = sc.ch;
            }

            else if( sc.Match(Logic::VerbatimStringLiteralStartCh1, Logic::VerbatimStringLiteralStartCh2) && m_isV8_0 )
            {
                sc.SetState(SCE_CSPRO_STRING);
                sc.Forward();

                extended_state = ( extended_state == ExtendedState::ExpectingTranslation ) ? ExtendedState::ProcessingTranslation :
                                                                                             ExtendedState::ProcessingMessageText;

                string_is_verbatim = true;
                string_quotemark = '"';
            }

            else if( !isspacechar(sc.ch) )
            {
                if( extended_state == ExtendedState::ExpectingTranslation )
                {
                    extended_state = ExtendedState::InvalidLine;
                }

                else
                {
                    sc.SetState(SCE_CSPRO_STRING);
                    extended_state = ExtendedState::ProcessingMessageText;

                    string_is_verbatim = true;
                    string_quotemark = 0;
                }
            }
        }

        // process the ) that follows the translation number/text
        else if( extended_state == ExtendedState::ExpectingRightParenthesis )
        {
            assert(sc.state == SCE_CSPRO_DEFAULT);

            if( sc.Match(')') )
            {
                extended_state = ExtendedState::ExpectingMessageText;
            }

            else if( !isspacechar(sc.ch) )
            {
                extended_state = ExtendedState::InvalidLine;
            }
        }

        // handle unexpected non-whitespace at the end of the line
        else if( extended_state == ExtendedState::ExpectingEndOfLine )
        {
            if( !isspacechar(sc.ch) )
                extended_state = ExtendedState::InvalidLine;
        }

        // process the message number
        else if( sc.state == SCE_CSPRO_NUMBER )
        {
            if( !IsADigit(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_DEFAULT);

                if( extended_state == ExtendedState::ProcessingTranslation )
                {
                    extended_state = ExtendedState::ExpectingRightParenthesis;
                    move_forward_at_end_of_while_loop = false;
                }

                else
                {
                    extended_state = isspacechar(sc.ch) ?  ExtendedState::ExpectingMessageText : 
                                                           ExtendedState::InvalidLine;
                }
            }
        }

        // if the line is invalid, color any invalid text using the default state
        if( extended_state == ExtendedState::InvalidLine && sc.state != SCE_CSPRO_DEFAULT )
            sc.SetState(SCE_CSPRO_DEFAULT);

        if( move_forward_at_end_of_while_loop )
        {
            sc.Forward();
        }

        else
        {
            move_forward_at_end_of_while_loop = true;
        }
    }

    sc.Complete();
}


namespace
{
    ILexer5* LexerFactoryMessage_V0()   { return new LexerCSProMessage(SCLEX_CSPRO_MESSAGE_V0); }
    ILexer5* LexerFactoryMessage_V8_0() { return new LexerCSProMessage(SCLEX_CSPRO_MESSAGE_V8_0); }
}

LexerModule lmCSProMessage_V0(SCLEX_CSPRO_MESSAGE_V0, LexerFactoryMessage_V0, LexCSPro::Name(SCLEX_CSPRO_MESSAGE_V0));
LexerModule lmCSProMessage_V8_0(SCLEX_CSPRO_MESSAGE_V8_0, LexerFactoryMessage_V8_0, LexCSPro::Name(SCLEX_CSPRO_MESSAGE_V8_0));
