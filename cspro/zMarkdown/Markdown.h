#pragma once

#include <zMarkdown/zMarkdown.h>


class ZMARKDOWN_API Markdown
{
public:
    // Converts Markdown to HTML.
    static std::string ToHtml(std::string_view markdown_sv);

    // Converts Markdown to a HTML document, wrapping the Markdown in a head (with the title) and a body.
    static std::string ToHtmlDocument(std::string_view title_sv, std::string_view markdown_sv);

private:
    static void AddToStringCallback(const char* text, unsigned int text_size, void* userdata);

    static void ToHtml(std::string& html, std::string_view markdown_sv);    
};
