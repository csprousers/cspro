#include "stdafx.h"
#include "LexerProperties.h"
#include <zLogicO/ReservedWords.h>


namespace LexerColor
{
    constexpr COLORREF Default         = RGB(0, 0, 0);
    constexpr COLORREF Comment         = RGB(0, 128, 0);
    constexpr COLORREF Operator        = RGB(0, 0, 0);

    constexpr COLORREF HtmlDefault     = RGB(70, 58, 61);
    constexpr COLORREF HtmlTag         = RGB(30, 170, 205);
    constexpr COLORREF HtmlAttribute   = RGB(91, 106, 109);
    constexpr COLORREF HtmlNumber      = RGB(132, 117, 216);
    constexpr COLORREF HtmlQuote       = RGB(71, 176, 65);

    // shared colors for JSON/YAML
    constexpr COLORREF JY_Number       = RGB(15, 75, 50);
    constexpr COLORREF JY_String       = RGB(100, 70, 175);
    constexpr COLORREF JY_PropertyName = RGB(170, 20, 100);
}


std::map<int, LexerProperties::Properties> LexerProperties::m_propertiesMap;


const LexerProperties::Properties& LexerProperties::GetProperties(int lexer_language)
{
    const auto& lookup = m_propertiesMap.find(lexer_language);

    if( lookup != m_propertiesMap.cend() )
        return lookup->second;

    Properties properties;

    for( const auto& [style, color] : GetColorsWorker(lexer_language) )
        properties.colors.try_emplace(static_cast<char>(style), color);

    GetKeywordsAndLogicTooltipsWorker(properties, lexer_language);

    return m_propertiesMap.try_emplace(lexer_language, std::move(properties)).first->second;
}


std::vector<std::tuple<int, COLORREF>> LexerProperties::GetColorsWorker(int lexer_language)
{
    if( Lexers::IsExternalLanguage(lexer_language) )
    {
        return GetExternalLanguageColorsWorker(lexer_language);
    }

    else if( lexer_language == SCLEX_NULL )
    {
        return { { STYLE_DEFAULT, LexerColor::Default } };
    }

    else
    {
        ASSERT(Lexers::IncorporatesCSProLogic(lexer_language) || Lexers::IsCSProMessage(lexer_language));

        return { // logic colors
                 { SCE_CSPRO_COMMENT,                    LexerColor::Comment },
                 { SCE_CSPRO_COMMENTLINE,                LexerColor::Comment },
                 { SCE_CSPRO_NUMBER,                     RGB(255, 0, 0) },
                 { SCE_CSPRO_STRING,                     RGB(255, 0, 255) },
                 { SCE_CSPRO_STRING_ESCAPE,              RGB(190, 0, 190) },
                 { SCE_CSPRO_KEYWORD,                    RGB(0, 0, 255) },
                 { SCE_CSPRO_DOT_NOTATION_FUNCTION,      RGB(0, 95, 200) },
                 { SCE_CSPRO_FUNCTION_NAMESPACE_PARENT,  RGB(0, 175, 200) },
                 { SCE_CSPRO_FUNCTION_NAMESPACE_CHILD,   RGB(0, 175, 200) },
                 { SCE_CSPRO_NAMED_ARGUMENT,             LexerColor::JY_PropertyName },
                                                         
                 // report colors                        
                 { SCE_CSPRO_REPORT_DEFAULT,             LexerColor::HtmlDefault },
                 { SCE_CSPRO_REPORT_MUSTACHE,            RGB(161, 126, 0) },
                 { SCE_CSPRO_REPORT_TRIP_MUSTACHE,       RGB(210, 82, 22) },
                 { SCE_CSPRO_REPORT_CSPROLOGIC,          RGB(216, 60, 135) },
                 { SCE_CSPRO_REPORT_HTML,                LexerColor::HtmlTag },
                 { SCE_CSPRO_REPORT_HTML_OPTION,         LexerColor::HtmlAttribute },
                 { SCE_CSPRO_REPORT_HTML_QUOTE,          LexerColor::HtmlQuote },
                 { SCE_CSPRO_REPORT_HTML_NUM,            LexerColor::HtmlNumber },

                 // document colors
                 { SCE_CSPRO_DOCUMENT_TAG,               LexerColor::HtmlTag },
                 { SCE_CSPRO_DOCUMENT_BOOLEAN_ATTRIBUTE, RGB(81, 141, 87) },
                 { SCE_CSPRO_DOCUMENT_ATTRIBUTE,         LexerColor::HtmlAttribute },
                 { SCE_CSPRO_DOCUMENT_VALUE,             LexerColor::HtmlQuote } };
    }
}


std::vector<std::tuple<int, COLORREF>> LexerProperties::GetExternalLanguageColorsWorker(int lexer_language)
{
    if( lexer_language == SCLEX_CSPRO_PRE80_SPEC_FILE )
    {
        constexpr COLORREF HeaderColor    = RGB(15, 70, 170);
        constexpr COLORREF AttributeColor = RGB(20, 102, 52);

        return { { SCE_CSPRO_PRE80_SPEC_FILE_DEFAULT,   LexerColor::Default },
                 { SCE_CSPRO_PRE80_SPEC_FILE_HEADER,    HeaderColor },
                 { SCE_CSPRO_PRE80_SPEC_FILE_ATTRIBUTE, AttributeColor } };
    }

    else if( lexer_language == SCLEX_HTML )
    {
        return { { SCE_H_DEFAULT,          LexerColor::HtmlDefault },
                 { SCE_H_TAG,              LexerColor::HtmlTag },
                 { SCE_H_TAGUNKNOWN,       LexerColor::HtmlTag },
                 { SCE_H_ATTRIBUTE,        LexerColor::HtmlAttribute },
                 { SCE_H_ATTRIBUTEUNKNOWN, LexerColor::HtmlAttribute },
                 { SCE_H_NUMBER,           LexerColor::HtmlNumber },
                 { SCE_H_DOUBLESTRING,     LexerColor::HtmlQuote },
                 { SCE_H_SINGLESTRING,     LexerColor::HtmlQuote },
                 { SCE_H_OTHER,            LexerColor::HtmlDefault },
                 { SCE_H_COMMENT,          LexerColor::Comment } };
    }

    else if( lexer_language == SCLEX_JAVASCRIPT || lexer_language == SCLEX_CPP )
    {
        constexpr COLORREF KeywordColor = RGB(140, 10, 200);
        constexpr COLORREF NumberColor  = LexerColor::Default;
        constexpr COLORREF StringColor  = RGB(160, 20, 20);

        return { { SCE_C_DEFAULT,     LexerColor::Default },
                 { SCE_C_COMMENT,     LexerColor::Comment },
                 { SCE_C_COMMENTLINE, LexerColor::Comment },
                 { SCE_C_NUMBER,      NumberColor },
                 { SCE_C_WORD,        KeywordColor },
                 { SCE_C_STRING,      StringColor },
                 { SCE_C_CHARACTER,   StringColor },
                 { SCE_C_OPERATOR,    LexerColor::Operator },
                 { SCE_C_STRINGEOL,   StringColor } };
    }

    else if( lexer_language == SCLEX_JSON )
    {
        constexpr COLORREF UriColor = RGB(110, 80, 185);

        return { { SCE_JSON_DEFAULT,      LexerColor::Default },
                 { SCE_JSON_NUMBER,       LexerColor::JY_Number },
                 { SCE_JSON_STRING,       LexerColor::JY_String },
                 { SCE_JSON_STRINGEOL,    LexerColor::JY_String },
                 { SCE_JSON_PROPERTYNAME, LexerColor::JY_PropertyName },
                 { SCE_JSON_OPERATOR,     LexerColor::Operator },
                 { SCE_JSON_URI,          UriColor },
                 { SCE_JSON_ERROR,        LexerColor::JY_Number } };
    }

    else if( lexer_language == SCLEX_PERCENT_ENCODING )
    {
        constexpr COLORREF PercentColor     = RGB(20, 155, 55);
        constexpr COLORREF HexColor         = PercentColor;
        constexpr COLORREF BadHexColor      = RGB(255, 90, 20);
        constexpr COLORREF BadNotUnreserved = RGB(199, 170, 60);

        return { { SCE_PERCENT_ENCODING_DEFAULT,            LexerColor::Default },
                 { SCE_PERCENT_ENCODING_PERCENT,            PercentColor },
                 { SCE_PERCENT_ENCODING_HEX,                HexColor },
                 { SCE_PERCENT_ENCODING_BAD_HEX,            BadHexColor },
                 { SCE_PERCENT_ENCODING_BAD_NOT_UNRESERVED, BadNotUnreserved } };
    }

    else if( lexer_language == SCLEX_SQL )
    {
        constexpr COLORREF KeywordColor = RGB(60, 0, 150);
        constexpr COLORREF NumberColor  = RGB(0, 150, 175);
        constexpr COLORREF StringColor  = RGB(255, 0, 0);

        return { { SCE_SQL_DEFAULT,     LexerColor::Default },
                 { SCE_SQL_COMMENT,     LexerColor::Comment },
                 { SCE_SQL_COMMENTLINE, LexerColor::Comment },
                 { SCE_SQL_NUMBER,      NumberColor },
                 { SCE_SQL_WORD,        KeywordColor },
                 { SCE_SQL_STRING,      StringColor },
                 { SCE_SQL_CHARACTER,   StringColor },
                 { SCE_SQL_OPERATOR,    LexerColor::Operator },
                 { SCE_SQL_IDENTIFIER,  StringColor } };
    }

    else if( lexer_language == SCLEX_YAML )
    {
        constexpr COLORREF DocumentColor = RGB(30, 0, 150);

        return { { SCE_YAML_DEFAULT,    LexerColor::Default },
                 { SCE_YAML_COMMENT,    LexerColor::Comment },
                 { SCE_YAML_IDENTIFIER, LexerColor::JY_PropertyName },
                 { SCE_YAML_KEYWORD,    LexerColor::Default},
                 { SCE_YAML_NUMBER,     LexerColor::JY_Number },
                 { SCE_YAML_REFERENCE,  LexerColor::Default },
                 { SCE_YAML_DOCUMENT,   DocumentColor },
                 { SCE_YAML_TEXT,       LexerColor::JY_String },
                 { SCE_YAML_ERROR,      LexerColor::Default },
                 { SCE_YAML_OPERATOR,   LexerColor::Operator } };
    }

    else
    {
        ASSERT(false);
        return { };
    }
}


void LexerProperties::GetKeywordsAndLogicTooltipsWorker(Properties& properties, int lexer_language)
{
    if( Lexers::IncorporatesCSProLogic(lexer_language) )
    {
        std::wstring keyword_lists[4];
        auto logic_tooltips = std::make_unique<std::map<StringNoCase, const TCHAR*>>();

        Logic::ReservedWords::ForeachReservedWord(
            [&](Logic::ReservedWords::ReservedWordType reserved_word_type, const std::wstring& reserved_word, const void* extra_information)
            {
                std::wstring* applicable_keyword_list;

                if( reserved_word_type == Logic::ReservedWords::ReservedWordType::Keyword ||
                    reserved_word_type == Logic::ReservedWords::ReservedWordType::AdditionalReservedWord )
                {
                    applicable_keyword_list = &keyword_lists[0];
                }

                else if( reserved_word_type == Logic::ReservedWords::ReservedWordType::Function )
                {
                    applicable_keyword_list = &keyword_lists[0];

                    const Logic::FunctionDetails* function_details = static_cast<const Logic::FunctionDetails*>(extra_information);
                    ASSERT(function_details != nullptr);

                    // add the tooltip
                    ASSERT(SO::StartsWithNoCase(function_details->tooltip, reserved_word));
                    logic_tooltips->try_emplace(StringNoCase(reserved_word), function_details->tooltip);
                }

                else if( reserved_word_type == Logic::ReservedWords::ReservedWordType::FunctionNamespace )
                {
                    applicable_keyword_list = &keyword_lists[1];
                }

                else if( reserved_word_type == Logic::ReservedWords::ReservedWordType::FunctionNamespaceChild )
                {
                    applicable_keyword_list = &keyword_lists[2];
                }

                else
                {
                    ASSERT(reserved_word_type == Logic::ReservedWords::ReservedWordType::FunctionDotNotation);
                    applicable_keyword_list = &keyword_lists[3];
                }

                // build the space delimited word (lowercase) string for Scintilla
                applicable_keyword_list->append(SO::ToLower(reserved_word));
                applicable_keyword_list->push_back(' ');
            });

        for( const std::wstring& list : keyword_lists )
            properties.keywords.emplace_back(UTF8Convert::WideToUTF8(list));

        properties.logic_tooltips = std::move(logic_tooltips);
    }

    else if( lexer_language == SCLEX_CPP )
    {
        // https://en.cppreference.com/w/cpp/keyword
        properties.keywords.emplace_back(
            "alignas alignof and and_eq asm auto bitand bitor bool break case catch char char8_t char16_t char32_t class "
            "compl concept const consteval constexpr constinit const_cast continue co_await co_return co_yield decltype "
            "default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline "
            "int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public register "
            "reinterpret_cast requires return short signed sizeof static static_assert static_cast struct switch template this "
            "thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq");
    }

    else if( lexer_language == SCLEX_JAVASCRIPT )
    {
        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Lexical_grammar
        properties.keywords.emplace_back(
            "abstract await boolean break byte case catch char class const continue debugger default delete do "
            "double else enum export extends false final finally float for function goto if implements import in "
            "instanceof int interface let long native new null package private protected public return short static "
            "super switch synchronized this throw throws transient true try typeof var void volatile while with yield");
    }

    else if( lexer_language == SCLEX_SQL )
    {
        // https://www.w3schools.com/sql/sql_ref_keywords.asp
        properties.keywords.emplace_back(
            "add all alter and any as asc backup between by case check column constraint create database default "
            "delete desc distinct drop exec exists foreign from full group having in index inner insert into is "
            "join key left like limit not null or order outer primary procedure replace right rownum select set "
            "table top truncate union unique update values view where");
    }

    else
    {
        ASSERT(lexer_language == SCLEX_CSPRO_MESSAGE_V0      || lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 ||
               lexer_language == SCLEX_CSPRO_PRE80_SPEC_FILE || lexer_language == SCLEX_HTML               ||
               lexer_language == SCLEX_JSON                  || lexer_language == SCLEX_PERCENT_ENCODING   ||
               lexer_language == SCLEX_YAML                  || lexer_language == SCLEX_NULL);
    }
}
