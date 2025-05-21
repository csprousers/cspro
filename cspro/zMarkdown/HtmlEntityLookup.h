#pragma once

#include <zMarkdown/zMarkdown.h>


class HtmlEntityLookup
{
public:
    virtual ~HtmlEntityLookup() { }

    // Instantiates a class to be used for lookups.
    ZMARKDOWN_API static std::unique_ptr<HtmlEntityLookup> Create();

    // Marked as virtual so that it is accessible from zHtml.
    virtual bool IsEntity(std::string_view text_sv);
};
