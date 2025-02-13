#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zScintilla/include/SciLexer.h>
#include <zAppO/LogicSettings.h>

class Application;


class Lexers
{
public:
    CLASS_DECL_ZEDIT2O static int GetLexerFromFilename(const std::wstring& filename);

    CLASS_DECL_ZEDIT2O static const TCHAR* GetLexerName(int lexer_language);

    CLASS_DECL_ZEDIT2O static const TCHAR* GetLexerDefaultServerMimeType(int lexer_language);


    static constexpr bool IsCSProLogic(int lexer_language)
    {
        return ( lexer_language == SCLEX_CSPRO_LOGIC_V0 ||
                 lexer_language == SCLEX_CSPRO_LOGIC_V8_0 );
    }


    static constexpr bool IsCSProMessage(int lexer_language)
    {
        return ( lexer_language == SCLEX_CSPRO_MESSAGE_V0 ||
                 lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 );
    }


    static constexpr bool IsCSProReportHtml(int lexer_language)
    {
        return ( lexer_language == SCLEX_CSPRO_REPORT_HTML_V0 ||
                 lexer_language == SCLEX_CSPRO_REPORT_HTML_V8_0 );
    }


    static constexpr bool IsCSProReport(int lexer_language)
    {
        return ( IsCSProReportHtml(lexer_language) || 
                 lexer_language == SCLEX_CSPRO_REPORT_V0 ||
                 lexer_language == SCLEX_CSPRO_REPORT_V8_0 );
    }


    static constexpr bool UsesCSProLogic(int lexer_language)
    {
        return ( IsCSProLogic(lexer_language) ||
                 IsCSProReport(lexer_language) );
    }


    static constexpr bool IncorporatesCSProLogic(int lexer_language)
    {
        return ( UsesCSProLogic(lexer_language) ||
                 lexer_language == SCLEX_CSPRO_DOCUMENT );
    }


    static constexpr bool IsExternalLanguage(int lexer_language)
    {
        switch( lexer_language )
        {
            case SCLEX_CSPRO_PRE80_SPEC_FILE:
            case SCLEX_CPP:
            case SCLEX_HTML:
            case SCLEX_JAVASCRIPT:
            case SCLEX_JSON:
            case SCLEX_PERCENT_ENCODING:
            case SCLEX_SQL:
            case SCLEX_YAML:
                return true;

            default:
                return false;
        }
    }


    static constexpr bool IsNotV0(int lexer_language)
    {
        switch( lexer_language )
        {
            case SCLEX_CSPRO_LOGIC_V0:
            case SCLEX_CSPRO_MESSAGE_V0:
            case SCLEX_CSPRO_REPORT_V0:
            case SCLEX_CSPRO_REPORT_HTML_V0:
                return false;

            default:
                return true;
        }
    }


    static constexpr bool CanFoldCode(int lexer_language)
    {
        return IsCSProLogic(lexer_language);
    }


    template<typename T>
    static int GetLexer_Logic(const T& application_or_logic_settings_or_version)
    {
        return UseV8_0Lexers(application_or_logic_settings_or_version) ? SCLEX_CSPRO_LOGIC_V8_0 :
                                                                         SCLEX_CSPRO_LOGIC_V0;
    }

    static constexpr int GetLexer_LogicV8_0()
    {
        return SCLEX_CSPRO_LOGIC_V8_0;
    }

    static constexpr int GetLexer_LogicDefault()
    {
        return SCLEX_CSPRO_LOGIC_V8_0;
    }


    template<typename T>
    static int GetLexer_Message(const T& application_or_logic_settings_or_version)
    {
        return UseV8_0Lexers(application_or_logic_settings_or_version) ? SCLEX_CSPRO_MESSAGE_V8_0 :
                                                                         SCLEX_CSPRO_MESSAGE_V0;
    }

    static constexpr int GetLexer_MessageDefault()
    {
        return SCLEX_CSPRO_MESSAGE_V8_0;
    }


    template<typename T>
    static int GetLexer_Report(const T& application_or_logic_settings_or_version, bool is_html_type)
    {
        if( is_html_type )
        {
            return UseV8_0Lexers(application_or_logic_settings_or_version) ? SCLEX_CSPRO_REPORT_HTML_V8_0 :
                                                                             SCLEX_CSPRO_REPORT_HTML_V0;
        }

        else
        {
            return UseV8_0Lexers(application_or_logic_settings_or_version) ? SCLEX_CSPRO_REPORT_V8_0 :
                                                                             SCLEX_CSPRO_REPORT_V0;
        }
    }


private:
    template<typename T>
    static bool UseV8_0Lexers(const T& application_or_logic_settings_or_version)
    {
        if constexpr(std::is_same_v<T, Application>)
        {
            return UseV8_0Lexers(application_or_logic_settings_or_version.GetLogicSettings());
        }

        else if constexpr(std::is_same_v<T, LogicSettings>)
        {
            return UseV8_0Lexers(application_or_logic_settings_or_version.GetVersion());
        }

        else
        {
            return ( application_or_logic_settings_or_version == LogicSettings::Version::V8_0 );
        }
    }
};
