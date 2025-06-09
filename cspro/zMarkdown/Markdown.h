#pragma once

#include <zMarkdown/zMarkdown.h>
#include <zHtml/CssProvider.h>


class ZMARKDOWN_API Markdown
{
public:
    // Converts Markdown to HTML.
    static std::string ToHtml(std::string_view markdown_sv);

    // Converts Markdown to a HTML document, wrapping the Markdown in a head (with the title) and a body.
    // The CSS is provided from the optional CssProvider. An exception is thrown if there is an error loading the CSS.
    static std::string ToHtmlDocument(std::string_view title_sv, std::string_view markdown_sv, CssProvider* css_provider = nullptr);

    // Creates a HTML document from Markdown that has already been converted to HTML.
    static std::string ToHtmlDocumentFromConvertedMarkdown(std::string_view title_sv, std::string_view html_sv, CssProvider* css_provider = nullptr);

    // Calls md_parse using the supplied ParserCallback subclass.
    // The ParserCallback's methods can all throw exceptions.
    class ParserCallback;
    class HtmlParserCallback;
    static void Parse(ParserCallback& parser_callback, std::string_view markdown_sv);

private:
    struct ParserCallbackWrapper;

    static void AddToStringCallback(const char* text, unsigned int text_size, void* userdata);

    static void ToHtml(std::string& html, std::string_view markdown_sv);

    template<typename CF>
    static std::string ToHtmlDocument(std::string_view title_sv, CssProvider* css_provider, const CF& callback_function);
};
