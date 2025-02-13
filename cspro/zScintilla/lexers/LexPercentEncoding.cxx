#include "LexCSPro.h"

using namespace Scintilla;
using namespace Lexilla;


class LexerPercentEncoding : public DefaultLexer 
{
public:
    static constexpr const char* LexerName = "percentencoding";

    LexerPercentEncoding();
    static ILexer5* CreateLexer();

    const char* SCI_METHOD PropertyGet(const char* key) override;
    int SCI_METHOD LineEndTypesSupported() override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
};


LexerPercentEncoding::LexerPercentEncoding()
    :   DefaultLexer(LexerName, SCLEX_PERCENT_ENCODING)
{
}


ILexer5* LexerPercentEncoding::CreateLexer()
{
    return new LexerPercentEncoding();
}


const char* LexerPercentEncoding::PropertyGet(const char* /*key*/)
{
    assert(false);
    return nullptr;
}


int LexerPercentEncoding::LineEndTypesSupported()
{
    return SC_LINE_END_TYPE_UNICODE;
}


void LexerPercentEncoding::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess)
{
    Accessor styler(pAccess, nullptr);
    StyleContext sc(startPos, length, initStyle, styler);

    enum class Section
    { 
        Default,
        Percent,
        Hex1,
        Hex2,
        NotUnreservedChar
    };

    Section section = Section::Default;

    for( ; sc.More(); sc.Forward() )
    {
        // start with the default coloring:
        // - on a newline
        // - after processing the second hex character
        // - after processing a not unreserved character
        if( sc.atLineStart || section == Section::Hex2 || section == Section::NotUnreservedChar )
        {
            sc.SetState(SCE_PERCENT_ENCODING_DEFAULT);
            section = Section::Default;
        }


        // switching from default to a percent
        if( section == Section::Default && sc.ch == '%' )
        {
            sc.SetState(SCE_PERCENT_ENCODING_PERCENT);
            section = Section::Percent;
        }

        // the first hex character
        else if( section == Section::Percent )
        {
            section = Section::Hex1;
        }

        // the second hex character
        else if( section == Section::Hex1 )
        {
            section = Section::Hex2;
        }

        // if a hex character, color it based on its validity
        if( section == Section::Hex1 || section == Section::Hex2 )
        {
            sc.SetState(IsADigit(sc.ch, 16) ? SCE_PERCENT_ENCODING_HEX : SCE_PERCENT_ENCODING_BAD_HEX);
        }

        // otherwise check if a character is not an unreserved character
        else if( section == Section::Default )
        {
            if( !( iswordchar(sc.ch) || sc.ch == '-' || sc.ch == '~' ) )
            {
                sc.SetState(SCE_PERCENT_ENCODING_BAD_NOT_UNRESERVED);
                section = Section::NotUnreservedChar;
            }
        }
    }

    sc.Complete();
}


LexerModule lmPercentEncoding(SCLEX_PERCENT_ENCODING, LexerPercentEncoding::CreateLexer, LexerPercentEncoding::LexerName);
