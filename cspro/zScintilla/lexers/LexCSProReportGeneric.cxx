#include "LexCSPro.h"
#include "PropSetSimple.h"

using namespace Scintilla;
using namespace Lexilla;

extern LexerModule lmCSProLogic_V0;
extern LexerModule lmCSProLogic_V8_0;
extern LexerModule lmMarkdown;


// --------------------------------------------------------------------------
// LexerCSProReportGeneric wraps two lexers, one that is used to style the
// report's underlying document type (e.g., Markdown), and one that styles
// the CSPro report tokens and logic.
//
// Limitations:
//    - PropSetSimple is not updated, so all properties will return their
//      default values.
//
//    - If the document lexer uses line states, ensure that this code works,
//      as the CSPro lexer uses lines states for storing information about
//      multiline comments and report tokens.
//
//    - The keyword list is only forwarded to the CSPro lexer, so if the
//      document lexer needs a keyword will, you must modify
//      LexerCSProReportGeneric::WordListSet.
//
//    - The document lexer's style codes must not overlap with CSPro's (91+).
// --------------------------------------------------------------------------

class LexerCSProReportGeneric : public DefaultLexer
{
public:
    LexerCSProReportGeneric(int language, const LexerModule& cspro_lexer_module, const LexerModule& document_lexer_module);
    ~LexerCSProReportGeneric();

    const char* SCI_METHOD PropertyGet(const char* key) override;
    int SCI_METHOD LineEndTypesSupported() override;
    Sci_Position SCI_METHOD WordListSet(int n, const char* wl) override;
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;

private:
    void LexDocument(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess, Accessor& styler);

private:
    LexCSPro* m_csproLexer;
    ILexer5* m_documentLexer;
    LexerFunction m_documentLexerFunction;
    PropSetSimple m_props;
};


LexerCSProReportGeneric::LexerCSProReportGeneric(const int language, const LexerModule& cspro_lexer_module, const LexerModule& document_lexer_module)
    :   DefaultLexer(LexCSPro::Name(language), language),
        m_csproLexer(static_cast<LexCSPro*>(cspro_lexer_module.Create())),
        m_documentLexer(document_lexer_module.Create()),
        m_documentLexerFunction(document_lexer_module.fnLexer)
{
    assert(document_lexer_module.wordListDescriptions == nullptr);
}


LexerCSProReportGeneric::~LexerCSProReportGeneric()
{
    m_csproLexer->Release();
    m_documentLexer->Release();
}


const char* LexerCSProReportGeneric::PropertyGet(const char* /*key*/)
{
    assert(false);
    return nullptr;
}


int LexerCSProReportGeneric::LineEndTypesSupported()
{
    return SC_LINE_END_TYPE_UNICODE;
}


Sci_Position LexerCSProReportGeneric::WordListSet(const int n, const char* const wl)
{
    return m_csproLexer->WordListSet(n, wl);
}


void LexerCSProReportGeneric::Lex(const Sci_PositionU startPos, const Sci_Position length, const int initStyle, IDocument* const pAccess)
{
    Accessor styler(pAccess, &m_props);
    assert(styler.Encoding() == EncodingType::unicode && pAccess == styler.MultiByteAccess());

    // process characters from [pos, end_pos)
    Sci_PositionU pos = startPos;
    const Sci_PositionU end_pos = pos + length;
    assert(end_pos <= static_cast<Sci_PositionU>(styler.Length()));
    int style = initStyle;

    // style any CSPro logic at the beginning of this text
    if( initStyle >= SCE_CSPRO_DEFAULT )
    {
        pos = m_csproLexer->Lex(pos, length, style, styler, true);
        style = 0;
    }

    int document_section_start_pos = pos;

    auto lex_document_section = [&]()
    {
        const Sci_Position document_length = pos - document_section_start_pos;
        assert(document_length >= 0);

        if( document_length != 0 )
        {
            LexDocument(document_section_start_pos, document_length, style, pAccess, styler);
            return true;
        }

        return false;
    };

    // while in the document, search for any characters that start a CSPro report section
    while( pos < end_pos )
    {
        const char ch = styler[pos];

        if( ( ch == '~' && styler.SafeGetCharAt(pos + 1) == '~' ) ||
            ( ch == '<' && styler.SafeGetCharAt(pos + 1) == '?' ) )
        {
            // style any document text
            if( lex_document_section() )
                style = static_cast<unsigned char>(pAccess->StyleAt(pos - 1));

            // temporarily clear the line state for the previous line, which CSProStyleContext uses;
            // this allows for the proper colorization of multiple report tokens on a single line; e.g.,:
            // <?
            //
            // ?> **bold** <? the start of this report section would not be handled properly without this code ?>
            const Sci_Position previous_line = pAccess->LineFromPosition(pos) - 1;
            const int saved_line_state = pAccess->SetLineState(previous_line, 0);

            // style the CSPro report tokens (and further logic)
            pos = m_csproLexer->Lex(pos, end_pos - pos, SCE_CSPRO_DEFAULT, styler, true);

            if( saved_line_state != 0 )
                pAccess->SetLineState(previous_line, saved_line_state);

            document_section_start_pos = pos;
        }

        else
        {
            ++pos;
        }
    }

    lex_document_section();
}


void LexerCSProReportGeneric::LexDocument(const Sci_PositionU startPos, const Sci_Position length, const int initStyle, IDocument* const pAccess, Accessor& styler)
{
    if( m_documentLexerFunction == nullptr )
    {
        m_documentLexer->Lex(startPos, length, initStyle, pAccess);
    }

    else
    {
        (*m_documentLexerFunction)(startPos, length, initStyle, nullptr, styler);
    }
}



namespace
{
    ILexer5* CreateLexerCSProReportMarkdown_V0()   { return new LexerCSProReportGeneric(SCLEX_CSPRO_REPORT_MARKDOWN_V0, lmCSProLogic_V0, lmMarkdown); }
    ILexer5* CreateLexerCSProReportMarkdown_V8_0() { return new LexerCSProReportGeneric(SCLEX_CSPRO_REPORT_MARKDOWN_V8_0, lmCSProLogic_V8_0, lmMarkdown); }
}

LexerModule lmCSProReportMarkdown_V0(SCLEX_CSPRO_REPORT_MARKDOWN_V0, CreateLexerCSProReportMarkdown_V0, LexCSPro::Name(SCLEX_CSPRO_REPORT_MARKDOWN_V0));
LexerModule lmCSProReportMarkdown_V8_0(SCLEX_CSPRO_REPORT_MARKDOWN_V8_0, CreateLexerCSProReportMarkdown_V8_0, LexCSPro::Name(SCLEX_CSPRO_REPORT_MARKDOWN_V8_0));
