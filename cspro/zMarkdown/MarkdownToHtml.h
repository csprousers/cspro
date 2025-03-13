#pragma once

#include <zMarkdown/zMarkdown.h>


class ZMARKDOWN_API Markdown
{
public:
    // Converts the markdown to HTML.
    static std::string ToHtml(std::string_view markdown_sv) noexcept;

    // Converts the markdown to a HTML document, wrapping the markdown in a head (with the title) and a body.
    static std::string ToHtmlDocument(std::string_view title_sv, std::string_view markdown_sv) noexcept;

private:
    static void ToHtml(std::string& html, std::string_view markdown_sv) noexcept;
};
