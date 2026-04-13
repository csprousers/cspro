#include "LexCSPro.h"
#include "LexerPercentEncoding.h"

using namespace Scintilla;
using namespace Lexilla;


namespace
{
    constexpr const char* LexerName = "property_string";
    constexpr int LexerLanguage     = SCLEX_CSPRO_PROPERTY_STRING;
}


class LexCSProPropertyString : public LexerPercentEncoding
{
protected:
    using LexerPercentEncoding::LexerPercentEncoding;

public:
    static ILexer5* CreateLexer();

    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
};


ILexer5* LexCSProPropertyString::CreateLexer()
{
    return new LexCSProPropertyString(LexerName, LexerLanguage);
}


void LexCSProPropertyString::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess)
{
    Accessor styler(pAccess, nullptr);
    StyleContext sc(startPos, length, initStyle, styler);

    assert(sc.atLineStart);

    while( sc.More() )
    {
        bool move_forward = true;

        // start with a new resource every newline
        if( sc.atLineStart )
            sc.SetState(SCE_CSPRO_PROPERTY_STRING_RESOURCE);

        // resource -> |
        if( sc.state == SCE_CSPRO_PROPERTY_STRING_RESOURCE )
        {
            if( sc.ch == '|' )
                sc.SetState(SCE_CSPRO_PROPERTY_STRING_PIPE);
        }

        // | -> attribute
        else if( sc.state == SCE_CSPRO_PROPERTY_STRING_PIPE )
        {
            sc.SetState(SCE_CSPRO_PROPERTY_STRING_ATTRIBUTE);
        }

        // attribute -> =
        // attribute -> &
        else if( sc.state == SCE_CSPRO_PROPERTY_STRING_ATTRIBUTE )
        {
            if( sc.ch == '=' )
            {
                sc.SetState(SCE_CSPRO_PROPERTY_STRING_EQUALS);
            }

            else if( sc.ch == '&' )
            {
                sc.SetState(SCE_CSPRO_PROPERTY_STRING_AMPERSAND);
            }
        }

        // = -> value
        else if( sc.state == SCE_CSPRO_PROPERTY_STRING_EQUALS )
        {
            sc.SetState(SCE_PERCENT_ENCODING_DEFAULT);
            LexPE(sc, true);

            assert(( sc.atLineStart && sc.state == SCE_CSPRO_PROPERTY_STRING_RESOURCE ) ||
                   ( sc.ch == '&' && sc.state == SCE_CSPRO_PROPERTY_STRING_AMPERSAND ) ||
                   ( sc.ch == '\0' ));

            // at the beginning of a line, don't move past this character
            // so that it is processed in the next iteration of the loop
            if( sc.atLineStart )
                move_forward = false;
        }

        // & -> attribute
        else if( sc.state == SCE_CSPRO_PROPERTY_STRING_AMPERSAND )
        {
            sc.SetState(SCE_CSPRO_PROPERTY_STRING_ATTRIBUTE);
        }

        if( move_forward )
            sc.Forward();
    }

    sc.Complete();
}


LexerModule lmCSProPropertyString(LexerLanguage, LexCSProPropertyString::CreateLexer, LexerName);
