#include "stdafx.h"
#include "Lexers.h"
#include <zUtilO/MimeType.h>


namespace
{
    struct LexerDetails
    {
        int lexer_language;
        const TCHAR* const lexer_name;
        std::vector<std::wstring> extensions;
    };

    const std::vector<LexerDetails>& GetLexerDetails()
    {
        static const std::vector<LexerDetails> lexer_details
        {
            { SCLEX_CSPRO_DOCUMENT,         _T("CSPro Document"),    { FileExtensions::CSDocument                                        } },
            { SCLEX_CSPRO_LOGIC_V8_0,       _T("CSPro Logic"),       { FileExtensions::Logic                                             } },
            { SCLEX_CSPRO_MESSAGE_V8_0,     _T("CSPro Messages"),    { FileExtensions::Message                                           } },
            { SCLEX_HTML,                   _T("HTML"),              { FileExtensions::HTML, FileExtensions::HTM, FileExtensions::CSHTML } },
            { SCLEX_JAVASCRIPT,             _T("JavaScript"),        { FileExtensions::JavaScript, FileExtensions::JavaScriptModule      } },
            { SCLEX_JSON,                   _T("JSON"),              { FileExtensions::Json, _T("geojson")                               } },
            { SCLEX_SQL,                    _T("SQL"),               { _T("sql")                                                         } },
            { SCLEX_YAML,                   _T("YAML"),              { FileExtensions::QuestionText                                      } },
        };

        return lexer_details;
    };
}


int Lexers::GetLexerFromFilename(const std::wstring& filename)
{
    const std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

    for( const LexerDetails& ld : GetLexerDetails() )
    {
        for( const std::wstring& this_extension : ld.extensions )
        {
            if( SO::EqualsNoCase(extension, this_extension) )
                return ld.lexer_language;
        }
    }

    return SCLEX_NULL;
}


const TCHAR* Lexers::GetLexerName(const int lexer_language)
{
    const std::vector<LexerDetails>& lexer_details = GetLexerDetails();
    const auto& lookup = std::find_if(lexer_details.cbegin(), lexer_details.cend(),
                                      [&](const LexerDetails& ld) { return ( ld.lexer_language == lexer_language ); });

    return ( lookup != lexer_details.cend() )                  ? lookup->lexer_name :
           ( lexer_language == SCLEX_CSPRO_LOGIC_V0 )          ? GetLexerName(SCLEX_CSPRO_LOGIC_V8_0) :
           ( lexer_language == SCLEX_CSPRO_MESSAGE_V0 )        ? GetLexerName(SCLEX_CSPRO_MESSAGE_V8_0) :
           ( lexer_language == SCLEX_CSPRO_REPORT_HTML_V8_0 ||
             lexer_language == SCLEX_CSPRO_REPORT_HTML_V0 )    ? _T("CSPro HTML Report") :
           ( lexer_language == SCLEX_CSPRO_REPORT_V8_0 ||
             lexer_language == SCLEX_CSPRO_REPORT_V0 )         ? _T("CSPro Report") :
           ( lexer_language == SCLEX_CSPRO_PRE80_SPEC_FILE )   ? _T("INI File") :
           ( lexer_language == SCLEX_PERCENT_ENCODING )        ? _T("Percent Encoding") :
           ( lexer_language == SCLEX_NULL )                    ? _T("Text") :
                                                                 ReturnProgrammingError(_T("Unknown"));
}


const TCHAR* Lexers::GetLexerDefaultServerMimeType(const int lexer_language)
{
    return ( lexer_language == SCLEX_HTML || IsCSProReportHtml(lexer_language) ) ? MimeType::Type::Html :
           ( lexer_language == SCLEX_JAVASCRIPT )                                ? MimeType::Type::JavaScript :
           ( lexer_language == SCLEX_JSON )                                      ? MimeType::Type::Json :
                                                                                   MimeType::ServerType::TextUtf8;
}
