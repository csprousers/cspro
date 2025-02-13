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

        // if not followed by a dot, see see if this is a keyword
        else if( next_ch != '.' && IsKeyword(sc, m_lexParameters.keywords) )
        {
            sc.ChangeState(SCE_CSPRO_KEYWORD);
            sc.SetState(SCE_CSPRO_DEFAULT);
            return;
        }
    }

    // see if this is a function namespace (a parent or child one)
    if( ( sc.state == SCE_CSPRO_IDENTIFIER && IsKeyword(sc, m_lexParameters.function_namespaces_parent) ) ||
        ( sc.state == SCE_CSPRO_IDENTIFIER_AFTER_FUNCTION_NAMESPACE_DOT && IsKeyword(sc, m_lexParameters.function_namespaces_child) ) )
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


void SCI_METHOD LexCSPro::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess)
{
    // No one likes a leaky string
    if( initStyle == SCE_CSPRO_STRING || initStyle == SCE_CSPRO_STRING_ESCAPE )
        initStyle = SCE_CSPRO_DEFAULT;

    Accessor styler(pAccess, NULL);
    StyleContext sc(startPos, length, initStyle, styler);

    // CSPro supports nested block comments!
    int commentNestLevel = styler.GetLineState(sc.currentLine - 1);

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

            styler.SetLineState(sc.currentLine, commentNestLevel);
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
                    if( commentNestLevel > 0 )
                        --commentNestLevel;

                    styler.SetLineState(sc.currentLine, commentNestLevel);

                    if( commentNestLevel == 0 )
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
                    ++commentNestLevel;
                    styler.SetLineState(sc.currentLine, commentNestLevel);
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
                    if( !string_is_verbatim && wcschr(Encoders::EscapeSequences, sc.ch) == nullptr )
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
                ++commentNestLevel;
                styler.SetLineState(sc.currentLine, commentNestLevel);
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
            else if( wcschr(Logic::OperatorCharacters, sc.ch) != nullptr )
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
}
