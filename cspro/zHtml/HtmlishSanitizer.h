#pragma once

#include <zHtml/zHtml.h>
#include <zMarkdown/HtmlEntityLookup.h>


// --------------------------------------------------------------------------
// HtmlishSanitizer
//
// This class takes text that may include tags as well as HTML entities.
// The output is proper HTML. For example:
//
// "abc & \n < <br> >"        ->  "abc &amp; <br> &lt; <br> &gt;"
// "<b> <bad></bad> </b>"     ->  "<b> &lt;bad&gt;&lt;/bad&gt; </b>"
// "&amp; &invalid; &#1234;"  ->  "&amp; &amp;invalid; &#1234;"
//
// No exceptions are thrown.
//
// Users of this class must also use zMarkdown.
// --------------------------------------------------------------------------

class HtmlishSanitizer
{
private:
    ZHTML_API HtmlishSanitizer(std::unique_ptr<HtmlEntityLookup> entity_lookup);

public:
    HtmlishSanitizer();

    ZHTML_API std::string Sanitize(cs::string_sz input);

private:
    class Worker;
    std::unique_ptr<HtmlEntityLookup> m_entityLookup;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline HtmlishSanitizer::HtmlishSanitizer()
    :   HtmlishSanitizer(HtmlEntityLookup::Create())
{
}
