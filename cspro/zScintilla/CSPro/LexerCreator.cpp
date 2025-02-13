#include "CSPro.h"

#include "SciLexer.h"
#include "Sci_Position.h"
#include "LexerModule.h"

#include <assert.h>


extern Lexilla::LexerModule lmNull;
extern Lexilla::LexerModule lmCSProLogic_V0;
extern Lexilla::LexerModule lmCSProLogic_V8_0;
extern Lexilla::LexerModule lmCSProMessage_V0;
extern Lexilla::LexerModule lmCSProMessage_V8_0;
extern Lexilla::LexerModule lmCSProReport_V0;
extern Lexilla::LexerModule lmCSProReport_V8_0;
extern Lexilla::LexerModule lmCSProReportHtml_V0;
extern Lexilla::LexerModule lmCSProReportHtmlV8_0;
extern Lexilla::LexerModule lmCSProPre80SpecFile;
extern Lexilla::LexerModule lmCSProDocument;
extern Lexilla::LexerModule lmCPP;
extern Lexilla::LexerModule lmHTML;
extern Lexilla::LexerModule lmJavaScript;
extern Lexilla::LexerModule lmJSON;
extern Lexilla::LexerModule lmPercentEncoding;
extern Lexilla::LexerModule lmSQL;
extern Lexilla::LexerModule lmYAML;


Scintilla::ILexer5* CSProScintilla::CreateLexer(int lexer_language)
{
    const Lexilla::LexerModule& lexer_module =
        ( lexer_language == SCLEX_NULL )                   ? lmNull :
        ( lexer_language == SCLEX_CSPRO_LOGIC_V0 )         ? lmCSProLogic_V0 :
        ( lexer_language == SCLEX_CSPRO_LOGIC_V8_0 )       ? lmCSProLogic_V8_0 :
        ( lexer_language == SCLEX_CSPRO_MESSAGE_V0 )       ? lmCSProMessage_V0 :
        ( lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 )     ? lmCSProMessage_V8_0 :
        ( lexer_language == SCLEX_CSPRO_REPORT_V0 )        ? lmCSProReport_V0 :
        ( lexer_language == SCLEX_CSPRO_REPORT_V8_0 )      ? lmCSProReport_V8_0 :
        ( lexer_language == SCLEX_CSPRO_REPORT_HTML_V0 )   ? lmCSProReportHtml_V0 :
        ( lexer_language == SCLEX_CSPRO_REPORT_HTML_V8_0 ) ? lmCSProReportHtmlV8_0 :
        ( lexer_language == SCLEX_CSPRO_PRE80_SPEC_FILE )  ? lmCSProPre80SpecFile :
        ( lexer_language == SCLEX_CSPRO_DOCUMENT )         ? lmCSProDocument :
        ( lexer_language == SCLEX_CPP )                    ? lmCPP :
        ( lexer_language == SCLEX_HTML )                   ? lmHTML :
        ( lexer_language == SCLEX_JAVASCRIPT )             ? lmJavaScript :
        ( lexer_language == SCLEX_JSON )                   ? lmJSON :
        ( lexer_language == SCLEX_PERCENT_ENCODING )       ? lmPercentEncoding :
        ( lexer_language == SCLEX_SQL )                    ? lmSQL :
        ( lexer_language == SCLEX_YAML )                   ? lmYAML :
                                                             lmNull;

    assert(&lexer_module != &lmNull || lexer_language == SCLEX_NULL);

    return lexer_module.Create();
}
