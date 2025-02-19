#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/LogicCtrl.h>
#include <iosfwd>

enum class SymbolType : int;
namespace Logic { struct FunctionNamespaceDetails; }


class CLASS_DECL_ZEDIT2O ScintillaColorizer
{
public:
    struct HtmlProcessor;

    // colorize an existing Scintilla control
    ScintillaColorizer(Scintilla::CScintillaCtrl& scintilla_ctrl, Sci_Position start_pos, Sci_Position end_pos);

    // colorize text that is not part of a Scintilla control
    ScintillaColorizer(int lexer_language, std::string_view text_sv);

    // colorize for HTML
    enum class HtmlProcessorType { FullHtml, SpanOnly, ContentOnly };
    std::string GetHtml(std::variant<HtmlProcessorType, HtmlProcessor*> html_processor_or_type);

    // colorize for the CSPro Users Forum
    std::string GetCSProUsersForumCode();

    // colorize for the CSPro Users Blog
    std::string GetCSProUsersBlogCode();

    // style color lookups
    COLORREF GetStyleColor(char style);
    const char* GetHtmlColor(COLORREF color);


    struct Entity
    {
        std::string text;
        char style;
    };

    struct ExtendedEntity
    {
        std::string text;
        char style;
        std::variant<std::monostate, SymbolType, const Logic::FunctionNamespaceDetails*> details;
        std::vector<std::tuple<std::string, std::string>> entity_spanning_tags;
        std::vector<std::tuple<std::string, std::string>> entity_specific_tags;
    };

    struct HtmlProcessor
    {
        virtual ~HtmlProcessor() { }

        virtual void WriteHtmlHeader(std::stringstream& output) const = 0;
        virtual void WriteHtmlFooter(std::stringstream& output) const = 0;

        virtual std::vector<ExtendedEntity> GetExtendedEntities(const std::vector<Entity>& /*entities*/) const { return {}; }
    };

private:
    void GenerateEntities(std::unique_ptr<char[]> chars_and_styles);

private:
    const std::map<char, COLORREF>& m_styleColorMap;
    std::vector<Entity> m_entities;
};
