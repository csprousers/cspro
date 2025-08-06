#pragma once

#include <zHtml/zHtml.h>


namespace HtmlConverter
{
    // Converts HTML to text. This is incredibly crude, with information
    // about what is converted detailed in the implementation.
    ZHTML_API std::string ToText(std::string html);

    // Converts HTML to Markdown, processing tags including <h1>, <strong>, etc.
    // Links, images, and lists will be converted.
    // Tables and other HTML elements will be returned as HTML.
    ZHTML_API std::string ToMarkdown(std::string html);
}
