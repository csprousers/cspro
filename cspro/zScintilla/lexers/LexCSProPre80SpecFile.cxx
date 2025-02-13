#include "LexCSPro.h"

using namespace Scintilla;
using namespace Lexilla;


class LexerCSProPre80SpecFile : public DefaultLexer 
{
public:
    static constexpr const char* LexerName = "csprospecfile";

    LexerCSProPre80SpecFile();
    static ILexer5* CreateLexer();

    const char* SCI_METHOD PropertyGet(const char* key) override;
    int SCI_METHOD LineEndTypesSupported() override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
};


LexerCSProPre80SpecFile::LexerCSProPre80SpecFile()
    :   DefaultLexer(LexerName, SCLEX_CSPRO_PRE80_SPEC_FILE)
{
}


ILexer5* LexerCSProPre80SpecFile::CreateLexer()
{
    return new LexerCSProPre80SpecFile();
}


const char* LexerCSProPre80SpecFile::PropertyGet(const char* /*key*/)
{
    assert(false);
    return nullptr;
}


int LexerCSProPre80SpecFile::LineEndTypesSupported()
{
    return SC_LINE_END_TYPE_UNICODE;
}


void LexerCSProPre80SpecFile::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess)
{
    Accessor styler(pAccess, nullptr);
    StyleContext sc(startPos, length, initStyle, styler);

    bool at_line_start = false;

    for( ; sc.More(); sc.Forward() )
    {
        // start with the default coloring on a newline
        if( sc.atLineStart )
        {
            sc.SetState(SCE_CSPRO_PRE80_SPEC_FILE_DEFAULT);
            at_line_start = true;
        }

        // ignore whitespace prior to the header or attribute start
        if( at_line_start && !isspacechar(sc.ch) )
        {
            at_line_start = false;

            sc.SetState(( sc.ch == '[' ) ? SCE_CSPRO_PRE80_SPEC_FILE_HEADER : 
                                           SCE_CSPRO_PRE80_SPEC_FILE_ATTRIBUTE);
        }

        else if( sc.state == SCE_CSPRO_PRE80_SPEC_FILE_ATTRIBUTE && sc.ch == '=' )
        {
            sc.SetState(SCE_CSPRO_PRE80_SPEC_FILE_DEFAULT);
        }
    }

    sc.Complete();
}


LexerModule lmCSProPre80SpecFile(SCLEX_CSPRO_PRE80_SPEC_FILE, LexerCSProPre80SpecFile::CreateLexer, LexerCSProPre80SpecFile::LexerName);
