#include "stdafx.h"
#include "LexerProperties.h"
#include <zToolsO/VectorHelpers.h>
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


const LexerProperties::Properties& LexerProperties::GetProperties(const int lexer_language)
{
    const auto& lookup = m_propertiesMap.find(lexer_language);

    if( lookup != m_propertiesMap.cend() )
        return lookup->second;

    Properties properties;

    for( const auto& [style_index, color] : GetStylesWorker(lexer_language) )
    {
        ASSERT(style_index >= 0 && style_index <= std::numeric_limits<unsigned char>::max());
        properties.styles.try_emplace(static_cast<unsigned char>(style_index), color);
    }

    GetKeywordsAndLogicTooltipsWorker(properties, lexer_language);

    return m_propertiesMap.try_emplace(lexer_language, std::move(properties)).first->second;
}


std::vector<std::tuple<int, LexerStyle>> LexerProperties::GetStylesWorker(const int lexer_language)
{
    if( Lexers::IsExternalLanguage(lexer_language) )
    {
        return GetExternalLanguageStylesWorker(lexer_language);
    }

    else if( lexer_language == SCLEX_NULL )
    {
        return { { STYLE_DEFAULT, { LexerColor::Default } } };
    }

    else
    {
        ASSERT(Lexers::IncorporatesCSProLogic(lexer_language) || Lexers::IsCSProMessage(lexer_language));

        std::vector<std::tuple<int, LexerStyle>> styles =
        {
            // logic colors
            { STYLE_DEFAULT,                        { LexerColor::Default } },
            { SCE_CSPRO_DEFAULT,                    { LexerColor::Default } },
            { SCE_CSPRO_COMMENT,                    { LexerColor::Comment } },
            { SCE_CSPRO_COMMENTLINE,                { LexerColor::Comment } },
            { SCE_CSPRO_NUMBER,                     { RGB(255, 0, 0) } },
            { SCE_CSPRO_STRING,                     { RGB(255, 0, 255) } },
            { SCE_CSPRO_STRING_ESCAPE,              { RGB(190, 0, 190) } },
            { SCE_CSPRO_KEYWORD,                    { RGB(0, 0, 255) } },
            { SCE_CSPRO_DOT_NOTATION_FUNCTION,      { RGB(0, 95, 200) } },
            { SCE_CSPRO_FUNCTION_NAMESPACE_PARENT,  { RGB(0, 175, 200) } },
            { SCE_CSPRO_FUNCTION_NAMESPACE_CHILD,   { RGB(0, 175, 200) } },
            { SCE_CSPRO_NAMED_ARGUMENT,             { LexerColor::JY_PropertyName } },

            // text template colors
            { SCE_CSPRO_REPORT_DOUBLE_TILDE,        { RGB(161, 126, 0) } },
            { SCE_CSPRO_REPORT_TRIPLE_TILDE,        { RGB(210, 82, 22) } },
            { SCE_CSPRO_REPORT_LOGIC_TAG,           { RGB(216, 60, 135) } },

            // HTML text template colors
            { SCE_CSPRO_REPORT_HTML_DEFAULT,        { LexerColor::HtmlDefault } },
            { SCE_CSPRO_REPORT_HTML_TAG,            { LexerColor::HtmlTag } },
            { SCE_CSPRO_REPORT_HTML_ATTR,           { LexerColor::HtmlAttribute } },
            { SCE_CSPRO_REPORT_HTML_QUOTE,          { LexerColor::HtmlQuote } },
            { SCE_CSPRO_REPORT_HTML_NUM,            { LexerColor::HtmlNumber } },

            // document colors
            { SCE_CSPRO_DOCUMENT_TAG,               { LexerColor::HtmlTag } },
            { SCE_CSPRO_DOCUMENT_BOOLEAN_ATTRIBUTE, { RGB(81, 141, 87) } },
            { SCE_CSPRO_DOCUMENT_ATTRIBUTE,         { LexerColor::HtmlAttribute } },
            { SCE_CSPRO_DOCUMENT_VALUE,             { LexerColor::HtmlQuote } },
        };

        // for Markdown text templates, add the Markdown colors
        if( Lexers::IsCSProTextTemplateMarkdown(lexer_language) )
            VectorHelpers::Append(styles, GetMarkdownStylesWorker());

        return styles;
    }
}


std::vector<std::tuple<int, LexerStyle>> LexerProperties::GetExternalLanguageStylesWorker(const int lexer_language)
{
    if( lexer_language == SCLEX_CSPRO_PRE80_SPEC_FILE )
    {
        constexpr COLORREF HeaderColor    = RGB(15, 70, 170);
        constexpr COLORREF AttributeColor = RGB(20, 102, 52);

        return
        {
            { SCE_CSPRO_PRE80_SPEC_FILE_DEFAULT,   { LexerColor::Default } },
            { SCE_CSPRO_PRE80_SPEC_FILE_HEADER,    { HeaderColor } },
            { SCE_CSPRO_PRE80_SPEC_FILE_ATTRIBUTE, { AttributeColor } },
        };
    }

    else if( lexer_language == SCLEX_HTML )
    {
        return
        {
            { SCE_H_DEFAULT,          { LexerColor::HtmlDefault } },
            { SCE_H_TAG,              { LexerColor::HtmlTag } },
            { SCE_H_TAGUNKNOWN,       { LexerColor::HtmlTag } },
            { SCE_H_ATTRIBUTE,        { LexerColor::HtmlAttribute } },
            { SCE_H_ATTRIBUTEUNKNOWN, { LexerColor::HtmlAttribute } },
            { SCE_H_NUMBER,           { LexerColor::HtmlNumber } },
            { SCE_H_DOUBLESTRING,     { LexerColor::HtmlQuote } },
            { SCE_H_SINGLESTRING,     { LexerColor::HtmlQuote } },
            { SCE_H_OTHER,            { LexerColor::HtmlDefault } },
            { SCE_H_COMMENT,          { LexerColor::Comment } },
        };
    }

    else if( lexer_language == SCLEX_JAVASCRIPT || lexer_language == SCLEX_CPP )
    {
        constexpr COLORREF KeywordColor = RGB(140, 10, 200);
        constexpr COLORREF NumberColor  = LexerColor::Default;
        constexpr COLORREF StringColor  = RGB(160, 20, 20);

        return
        {
            { SCE_C_DEFAULT,     { LexerColor::Default } },
            { SCE_C_COMMENT,     { LexerColor::Comment } },
            { SCE_C_COMMENTLINE, { LexerColor::Comment } },
            { SCE_C_NUMBER,      { NumberColor } },
            { SCE_C_WORD,        { KeywordColor } },
            { SCE_C_STRING,      { StringColor } },
            { SCE_C_CHARACTER,   { StringColor } },
            { SCE_C_OPERATOR,    { LexerColor::Operator } },
            { SCE_C_STRINGEOL,   { StringColor } },
        };
    }

    else if( lexer_language == SCLEX_JSON )
    {
        constexpr COLORREF UriColor = RGB(110, 80, 185);

        return
        {
            { SCE_JSON_DEFAULT,      { LexerColor::Default } },
            { SCE_JSON_NUMBER,       { LexerColor::JY_Number } },
            { SCE_JSON_STRING,       { LexerColor::JY_String } },
            { SCE_JSON_STRINGEOL,    { LexerColor::JY_String } },
            { SCE_JSON_PROPERTYNAME, { LexerColor::JY_PropertyName } },
            { SCE_JSON_OPERATOR,     { LexerColor::Operator } },
            { SCE_JSON_URI,          { UriColor } },
            { SCE_JSON_ERROR,        { LexerColor::JY_Number } },
        };
    }

    else if( lexer_language == SCLEX_MARKDOWN )
    {
        return GetMarkdownStylesWorker();
    }

    else if( const bool is_ps = ( lexer_language == SCLEX_CSPRO_PROPERTY_STRING );
             is_ps || lexer_language == SCLEX_PERCENT_ENCODING )
    {
        constexpr COLORREF PercentColor     = RGB(20, 155, 55);
        constexpr COLORREF HexColor         = PercentColor;
        constexpr COLORREF BadHexColor      = RGB(255, 90, 20);
        constexpr COLORREF BadNotUnreserved = RGB(199, 170, 60);
        constexpr COLORREF PS_Attribute     = RGB(10, 90, 45);
        constexpr COLORREF PS_Value         = RGB(15, 120, 45);
        constexpr COLORREF PS_Control       = RGB(10, 65, 25);

        std::vector<std::tuple<int, LexerStyle>> styles =
        {
            { SCE_PERCENT_ENCODING_DEFAULT,            { is_ps ? PS_Value : LexerColor::Default } },
            { SCE_PERCENT_ENCODING_PERCENT,            { PercentColor } },
            { SCE_PERCENT_ENCODING_HEX,                { HexColor } },
            { SCE_PERCENT_ENCODING_BAD_HEX,            { BadHexColor } },
            { SCE_PERCENT_ENCODING_BAD_NOT_UNRESERVED, { BadNotUnreserved } },
        };

        // for property strings, add a few more styles
        if( is_ps )
        {
            styles.emplace_back(SCE_CSPRO_PROPERTY_STRING_RESOURCE, LexerStyle { LexerColor::Default });
            styles.emplace_back(SCE_CSPRO_PROPERTY_STRING_ATTRIBUTE, LexerStyle { PS_Attribute, LexerStyle::NoOverride, true, false });
            styles.emplace_back(SCE_CSPRO_PROPERTY_STRING_PIPE, LexerStyle { PS_Control });
            styles.emplace_back(SCE_CSPRO_PROPERTY_STRING_EQUALS, LexerStyle { PS_Control });
            styles.emplace_back(SCE_CSPRO_PROPERTY_STRING_AMPERSAND, LexerStyle { PS_Control });
        }

        return styles;
    }

    else if( lexer_language == SCLEX_SQL )
    {
        constexpr COLORREF KeywordColor = RGB(60, 0, 150);
        constexpr COLORREF NumberColor  = RGB(0, 150, 175);
        constexpr COLORREF StringColor  = RGB(255, 0, 0);

        return
        {
            { SCE_SQL_DEFAULT,     { LexerColor::Default } },
            { SCE_SQL_COMMENT,     { LexerColor::Comment } },
            { SCE_SQL_COMMENTLINE, { LexerColor::Comment } },
            { SCE_SQL_NUMBER,      { NumberColor } },
            { SCE_SQL_WORD,        { KeywordColor } },
            { SCE_SQL_STRING,      { StringColor } },
            { SCE_SQL_CHARACTER,   { StringColor } },
            { SCE_SQL_OPERATOR,    { LexerColor::Operator } },
            { SCE_SQL_IDENTIFIER,  { StringColor } },
        };
    }

    else if( lexer_language == SCLEX_YAML )
    {
        constexpr COLORREF DocumentColor = RGB(30, 0, 150);

        return
        {
            { SCE_YAML_DEFAULT,    { LexerColor::Default } },
            { SCE_YAML_COMMENT,    { LexerColor::Comment } },
            { SCE_YAML_IDENTIFIER, { LexerColor::JY_PropertyName } },
            { SCE_YAML_KEYWORD,    { LexerColor::Default} },
            { SCE_YAML_NUMBER,     { LexerColor::JY_Number } },
            { SCE_YAML_REFERENCE,  { LexerColor::Default } },
            { SCE_YAML_DOCUMENT,   { DocumentColor } },
            { SCE_YAML_TEXT,       { LexerColor::JY_String } },
            { SCE_YAML_ERROR,      { LexerColor::Default } },
            { SCE_YAML_OPERATOR,   { LexerColor::Operator } },
        };
    }

    else
    {
        ASSERT(false);
        return { };
    }
}


std::vector<std::tuple<int, LexerStyle>> LexerProperties::GetMarkdownStylesWorker()
{
    constexpr COLORREF BoldColor           = RGB(70, 130, 180);
    constexpr COLORREF ItalicColor         = RGB(185, 135, 10);
    constexpr COLORREF HeaderColor         = RGB(0, 135, 50);
    constexpr COLORREF BulletsColor        = RGB(30, 0, 150);
    constexpr COLORREF LinkColor           = RGB(30, 145, 255);
    constexpr COLORREF BlockquoteColor     = RGB(75, 75, 150);
    constexpr COLORREF StrikeoutColor      = RGB(128, 128, 128);
    constexpr COLORREF HorizontalRuleColor = RGB(30, 30, 255);
    constexpr COLORREF CodeForeColor       = RGB(90, 50, 100);
    constexpr COLORREF CodeBackColor       = RGB(250, 245, 255);

    return
    {
        { SCE_MARKDOWN_DEFAULT,    { LexerColor::Default } },
        { SCE_MARKDOWN_STRONG1,    { BoldColor, LexerStyle::NoOverride, true, false } },
        { SCE_MARKDOWN_STRONG2,    { BoldColor, LexerStyle::NoOverride, true, false } },
        { SCE_MARKDOWN_EM1,        { ItalicColor, LexerStyle::NoOverride, false, true } },
        { SCE_MARKDOWN_EM2,        { ItalicColor, LexerStyle::NoOverride, false, true } },
        { SCE_MARKDOWN_HEADER1,    { HeaderColor } },
        { SCE_MARKDOWN_HEADER2,    { HeaderColor } },
        { SCE_MARKDOWN_HEADER3,    { HeaderColor } },
        { SCE_MARKDOWN_HEADER4,    { HeaderColor } },
        { SCE_MARKDOWN_HEADER5,    { HeaderColor } },
        { SCE_MARKDOWN_HEADER6,    { HeaderColor } },
        { SCE_MARKDOWN_PRECHAR,    { LexerColor::Default } },
        { SCE_MARKDOWN_ULIST_ITEM, { BulletsColor } },
        { SCE_MARKDOWN_OLIST_ITEM, { BulletsColor } },
        { SCE_MARKDOWN_BLOCKQUOTE, { BlockquoteColor } },
        { SCE_MARKDOWN_STRIKEOUT,  { StrikeoutColor } },
        { SCE_MARKDOWN_HRULE,      { HorizontalRuleColor } },
        { SCE_MARKDOWN_LINK,       { LinkColor } },
        { SCE_MARKDOWN_CODE,       { CodeForeColor, CodeBackColor, false, false } },
        { SCE_MARKDOWN_CODE2,      { CodeForeColor, CodeBackColor, false, false } },
        { SCE_MARKDOWN_CODEBK,     { CodeForeColor, CodeBackColor, false, false } },
    };
}


void LexerProperties::GetKeywordsAndLogicTooltipsWorker(Properties& properties, const int lexer_language)
{
    if( Lexers::IncorporatesCSProLogic(lexer_language) )
    {
        std::string keyword_lists[4];
        auto logic_tooltips = std::make_unique<std::map<std::string, const char*, cs::case_insensitive_less>>();

        Logic::ReservedWords::ForeachReservedWord(
            [&](const Logic::ReservedWords::ReservedWordType reserved_word_type, const std::string& reserved_word, const void* const extra_information)
            {
                std::string* applicable_keyword_list;

                if( reserved_word_type == Logic::ReservedWords::ReservedWordType::Keyword ||
                    reserved_word_type == Logic::ReservedWords::ReservedWordType::AdditionalReservedWord )
                {
                    applicable_keyword_list = &keyword_lists[0];
                }

                else if( reserved_word_type == Logic::ReservedWords::ReservedWordType::Function )
                {
                    applicable_keyword_list = &keyword_lists[0];

                    const Logic::FunctionDetails* const function_details = static_cast<const Logic::FunctionDetails*>(extra_information);
                    ASSERT(function_details != nullptr);

                    // add the tooltip
                    ASSERT(SO::StartsWithNoCase(function_details->tooltip, reserved_word));
                    logic_tooltips->try_emplace(reserved_word, function_details->tooltip);
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

        for( const std::string& list : keyword_lists )
            properties.keywords.emplace_back(list);

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
        ASSERT(lexer_language == SCLEX_CSPRO_MESSAGE_V0 ||
               lexer_language == SCLEX_CSPRO_MESSAGE_V8_0 ||
               lexer_language == SCLEX_CSPRO_PRE80_SPEC_FILE ||
               lexer_language == SCLEX_CSPRO_PROPERTY_STRING ||
               lexer_language == SCLEX_HTML ||
               lexer_language == SCLEX_JSON ||
               lexer_language == SCLEX_MARKDOWN ||
               lexer_language == SCLEX_PERCENT_ENCODING ||
               lexer_language == SCLEX_YAML ||
               lexer_language == SCLEX_NULL);
    }
}
