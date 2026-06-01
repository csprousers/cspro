#include "LexCSPro.h"
#include <zToolsO/EscapesAndLogicOperators.h>

using namespace Scintilla;
using namespace Lexilla;


namespace
{
    enum class NumType { Decimal, FormatError };

    constexpr int GetNumStyle(NumType num_type) noexcept
    {
        return ( num_type == NumType::FormatError ) ? SCE_CSPRO_NUMERROR :
                                                      SCE_CSPRO_NUMBER;
    }

    constexpr bool IsIdentifierState(int state)
    {
        return ( state == SCE_CSPRO_IDENTIFIER ||
                 state == SCE_CSPRO_IDENTIFIER_AFTER_DOT ||
                 state == SCE_CSPRO_IDENTIFIER_AFTER_FUNCTION_NAMESPACE_DOT );
    }
}


Sci_Position SCI_METHOD LexCSPro::WordListSet(int n, const char* wl)
{
    WordList* wordListN = ( n == 0 ) ? &m_lexParameters.keywords :
                          ( n == 1 ) ? &m_lexParameters.function_namespaces_parent :
                          ( n == 2 ) ? &m_lexParameters.function_namespaces_child :
                          ( n == 3 ) ? &m_lexParameters.dot_notation_functions :
                                       nullptr;
    assert(wordListN != nullptr);

    if( wordListN != nullptr )
    {
        WordList wlNew;
        wlNew.Set(wl);

        if( *wordListN != wlNew )
        {
            wordListN->Set(wl);
            return 0;
        }
    }

    return -1;
}


void LexCSPro::SetPostIdentifierState(Lexilla::StyleContext& sc)
{
    assert(IsIdentifierState(sc.state));

    // get the next non-whitespace character
    Sci_Position next_ch_offset = 0;
    char next_ch;

    while( ( next_ch = sc.GetRelative(next_ch_offset) ) != 0 && isspacechar(next_ch) )
        ++next_ch_offset;

    if( sc.state == SCE_CSPRO_IDENTIFIER )
    {
        // if an identifier is followed by the named argument operator, treat it as a named argument
        if( next_ch == ':' && sc.GetRelative(next_ch_offset + 1) == '=' )
        {
            sc.ChangeState(SCE_CSPRO_NAMED_ARGUMENT);
            sc.SetState(SCE_CSPRO_DEFAULT);
            return;
        }

        // if not followed by a dot, see if this is a keyword
        else if( next_ch != '.' && IsKeyword(sc, m_lexParameters.keywords) )
        {
            sc.ChangeState(SCE_CSPRO_KEYWORD);
            sc.SetState(SCE_CSPRO_DEFAULT);
            return;
        }
    }

    // see if this is a function namespace (a parent or child one)
    bool is_function_namespace = false;

    if( sc.state == SCE_CSPRO_IDENTIFIER )
    {
        is_function_namespace = IsKeyword(sc, m_lexParameters.function_namespaces_parent);
    }

    else if( sc.state == SCE_CSPRO_IDENTIFIER_AFTER_FUNCTION_NAMESPACE_DOT )
    {
        if( IsKeyword(sc, m_lexParameters.function_namespaces_child) )
        {
            // because Sync is a namespace, make sure that the "sync" of CS.Data.sync
            // is treated as a dot-notation function, not a namespace;
            // we can check this by seeing if "Sync" was used as opposed to "sync"
            if( strcmp(m_keywordCheckBuffer, "sync") == 0 )
            {
                static_assert(_countof(m_keywordCheckBuffer) >= 2);
                sc.GetCurrent(m_keywordCheckBuffer, 2);
                is_function_namespace = ( m_keywordCheckBuffer[0] == 'S' );
            }

            else
            {
                is_function_namespace = true;
            }
        }
    }

    if( is_function_namespace )
    {
        sc.ChangeState(( sc.state == SCE_CSPRO_IDENTIFIER ) ? SCE_CSPRO_FUNCTION_NAMESPACE_PARENT :
                                                              SCE_CSPRO_FUNCTION_NAMESPACE_CHILD);
        sc.SetState(SCE_CSPRO_DEFAULT);

        // skip past whitespace and the dot to determine if this is followed by an identifier,
        // in which case it will likely be a dot-notation function
        while( sc.ch != '.' && isspacechar(sc.ch) && sc.More() )
            sc.Forward();

        if( sc.Match('.') )
        {
            while( sc.More() )
            {
                sc.Forward();

                if( IsCSProWordStart(sc.ch) )
                {
                    sc.SetState(SCE_CSPRO_IDENTIFIER_AFTER_FUNCTION_NAMESPACE_DOT);
                    break;
                }

                if( !isspacechar(sc.ch) )
                    break;
            }
        }

        return;
    }

    // see if this is a dot-notation function
    if( ( sc.state == SCE_CSPRO_IDENTIFIER_AFTER_DOT || sc.state == SCE_CSPRO_IDENTIFIER_AFTER_FUNCTION_NAMESPACE_DOT ) &&
        IsKeyword(sc, m_lexParameters.dot_notation_functions) )
    {
        sc.ChangeState(SCE_CSPRO_DOT_NOTATION_FUNCTION);
        sc.SetState(SCE_CSPRO_DEFAULT);
        return;
    }

    sc.ChangeState(SCE_CSPRO_IDENTIFIER);
    sc.SetState(SCE_CSPRO_DEFAULT);
}


void SCI_METHOD LexCSPro::Lex(const Sci_PositionU startPos, const Sci_Position length, const int initStyle, IDocument* const pAccess)
{
    Accessor styler(pAccess, nullptr);
    Lex(startPos, length, initStyle, styler, false);
}


Sci_PositionU LexCSPro::Lex(const Sci_PositionU startPos, const Sci_Position length, int initStyle, Accessor& styler, const bool process_report_tokens)
{
    // No one likes a leaky string
    if( initStyle == SCE_CSPRO_STRING || initStyle == SCE_CSPRO_STRING_ESCAPE )
        initStyle = SCE_CSPRO_DEFAULT;

    CSProStyleContext sc(startPos, length, initStyle, styler);

    NumType numType = NumType::Decimal;
    int decimalCount = 0;
    int string_quotemark = 0;
    bool string_is_verbatim = false;
    bool move_forward_at_end_of_while_loop = true;

    while( sc.More() )
    {
        if( sc.atLineStart )
        {
            if( sc.state == SCE_CSPRO_STRING )
                sc.SetState(SCE_CSPRO_STRING);

            sc.UpdateLineState();
        }

        switch( sc.state )
        {
            case SCE_CSPRO_OPERATOR:
            {
                sc.SetState(SCE_CSPRO_DEFAULT);
                break;
            }

            case SCE_CSPRO_NUMBER:
            {
                if( sc.Match('.') )
                {
                    if( sc.chNext == '.' )
                    {
                        // Pass
                    }

                    else
                    {
                        ++decimalCount;

                        if( numType == NumType::Decimal )
                        {
                            if( decimalCount <= 1 && !IsCSProWordChar(sc.chNext) )
                                break;
                        }
                    }
                }

                else if( numType == NumType::Decimal )
                {
                    if( IsADigit(sc.ch) )
                        break;
                }

                else if( IsADigit(sc.ch) )
                {
                    numType = NumType::FormatError;
                    break;
                }

                sc.ChangeState(GetNumStyle(numType));
                sc.SetState(SCE_CSPRO_DEFAULT);
                break;
            }

            case SCE_CSPRO_COMMENT:
            {
                if( IsV8_0() ? sc.Match('*', '/') : sc.Match('}') )
                {
                    if( sc.DecrementCommentNestLevel() == 0 )
                    {
                        //to fix the bug when the character after the closing comment is still colored in comment color
                        sc.Forward();

                        if( IsV8_0() )
                            sc.Forward();

                        sc.SetState(SCE_CSPRO_DEFAULT);
                    }
                }

                else if( IsV8_0() ? sc.Match('/', '*') : sc.Match('{') )
                {
                    sc.IncrementCommentNestLevel();
                }

                break;
            }

            case SCE_CSPRO_COMMENTLINE:
            {
                if( sc.atLineStart )
                    sc.SetState(SCE_CSPRO_DEFAULT);

                break;
            }

            case SCE_CSPRO_STRING:
            {
                // verbatim string literal escapes are double quoted: ""
                // otherwise they are escaped with a backslash: \[char]
                if( string_is_verbatim ? ( sc.Match('"', '"') ) :
                                         ( sc.Match('\\') && IsV8_0() ) )
                {
                    sc.SetState(SCE_CSPRO_STRING_ESCAPE);
                }

                // string quotes ending
                else if( sc.Match(string_quotemark) )
                {
                    sc.ForwardSetState(SCE_CSPRO_DEFAULT);
                }

                // the line ending without the string literal properly closed
                else if( sc.atLineEnd )
                {
                    sc.ForwardSetState(SCE_CSPRO_DEFAULT);
                }

                break;
            }

            case SCE_CSPRO_STRING_ESCAPE:
            {
                if( sc.atLineEnd )
                {
                    sc.ForwardSetState(SCE_CSPRO_DEFAULT);
                }

                else
                {
                    assert(sc.chPrev == ( string_is_verbatim ? '"' : '\\' ));

                    // if an escape (for a normal string literal) is not valid, color it as a normal string
                    if( !string_is_verbatim && strchr(EncoderEscapes::Sequences, sc.ch) == nullptr )
                        sc.ChangeState(SCE_CSPRO_STRING);

                    sc.ForwardSetState(SCE_CSPRO_STRING);
                    move_forward_at_end_of_while_loop = false;
                }

                break;
            }
        }


        // identifier state processing
        if( IsIdentifierState(sc.state) )
        {
            if( !IsCSProWordChar(sc.ch) )
                SetPostIdentifierState(sc);
        }

        // default state processing
        if( sc.state == SCE_CSPRO_DEFAULT )
        {
            // number
            if( IsADigit(sc.ch) || ( sc.Match('.') && IsADigit(sc.chNext) ) )
            {
                sc.SetState(SCE_CSPRO_NUMBER);

                numType = NumType::Decimal;
                decimalCount = 0;
            }

            // string literal
            else if( is_quotemark(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_STRING);

                string_is_verbatim = false;
                string_quotemark = sc.ch;
            }

            // verbatim string literal
            else if( sc.Match(Logic::VerbatimStringLiteralStartCh1, Logic::VerbatimStringLiteralStartCh2) && IsV8_0() )
            {
                sc.SetState(SCE_CSPRO_STRING);
                sc.Forward();

                string_is_verbatim = true;
                string_quotemark = '"';
            }

            // keyword
            else if( IsCSProWordStart(sc.ch) )
            {
                sc.SetState(SCE_CSPRO_IDENTIFIER);
            }

            // dot notation start
            else if( sc.Match('.') )
            {
                // skip past whitespace to determine if this followed by a identifier
                while( sc.More() )
                {
                    sc.Forward();

                    if( IsCSProWordStart(sc.ch) )
                    {
                        sc.SetState(SCE_CSPRO_IDENTIFIER_AFTER_DOT);
                        break;
                    }

                    else if( !isspacechar(sc.ch) )
                    {
                        break;
                    }
                }
            }

            // comments
            else if( IsV8_0() ? sc.Match('/', '*') : sc.Match('{') )
            {
                sc.IncrementCommentNestLevel();
                sc.SetState(SCE_CSPRO_COMMENT);

                if( IsV8_0() )
                    sc.Forward();
            }

            // single line CSPro comment
            else if( sc.Match('/', '/') )
            {
                sc.SetState(SCE_CSPRO_COMMENTLINE);
            }

            // operators
            else if( ( strchr(Logic::OperatorCharacters, sc.ch) != nullptr ) &&
                     ( !process_report_tokens || !sc.Match('<', '?') ) )
            {
                // the operators exclude $ and . because those should be counted as identifiers
                sc.SetState(SCE_CSPRO_OPERATOR);

                // Ignore decimal coloring in input like: range[0..5]
                if( sc.Match('.', '.') )
                {
                    sc.Forward();

                    if( sc.chNext == '.' )
                        sc.Forward();
                }
            }

            // process report tokens: ~~ ~~~ <? ?>
            else if( process_report_tokens )
            {
                const unsigned char current_report_style = sc.GetReportStyle();
                unsigned char new_report_style = 0;

                if( current_report_style == SCE_CSPRO_REPORT_LOGIC_TAG )
                {
                    if( sc.Match('?', '>') )
                        new_report_style = SCE_CSPRO_REPORT_LOGIC_TAG;
                }

                else if( sc.Match('~', '~') )
                {
                    if( ( current_report_style == 0 || current_report_style == SCE_CSPRO_REPORT_TRIPLE_TILDE ) &&
                        ( sc.GetRelativeCharacter(2) == '~' ) )
                    {
                        new_report_style = SCE_CSPRO_REPORT_TRIPLE_TILDE;
                    }

                    else if( current_report_style == 0 || current_report_style == SCE_CSPRO_REPORT_DOUBLE_TILDE )
                    {
                        new_report_style = SCE_CSPRO_REPORT_DOUBLE_TILDE;
                    }
                }

                else if( current_report_style == 0 && sc.Match('<', '?') )
                {
                    new_report_style = SCE_CSPRO_REPORT_LOGIC_TAG;
                }

                // if starting or ending a report section, style the report tokens
                if( new_report_style != 0 )
                {
                    sc.SetState(new_report_style);
                    sc.Forward(( new_report_style == SCE_CSPRO_REPORT_TRIPLE_TILDE ) ? 3 : 2);
                    sc.SetState(SCE_CSPRO_DEFAULT);

                    // if ending a report section, return to the calling lexer
                    if( current_report_style != 0 )
                    {
                        sc.ClearReportStyle();
                        break;
                    }

                    else
                    {
                        sc.SetReportStyle(new_report_style);
                        move_forward_at_end_of_while_loop = false;
                    }
                }
            }
        }

        if( move_forward_at_end_of_while_loop )
        {
            sc.Forward();
        }

        else
        {
            move_forward_at_end_of_while_loop = true;
        }
    }


    // this final check is necessary when, for example, this method is called from the report lexer
    if( IsIdentifierState(sc.state) && IsCSProWordChar(sc.chPrev) )
        SetPostIdentifierState(sc);

    sc.Complete();

    return sc.currentPos;
}
