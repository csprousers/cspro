#pragma once

#include <zHtml/zHtml.h>


class ZHTML_API HtmlTemplates
{
public:
    // Returns the HTML for a page with the text centered both vertically and horizontally.
    static std::string CreateCenteredTextPage(std::string_view title_sv, std::string_view text_sv);
};
