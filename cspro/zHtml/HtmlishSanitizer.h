#pragma once

#include <zHtml/zHtml.h>


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
// --------------------------------------------------------------------------

class HtmlishSanitizer
{
    class Worker;

public:
    template<typename IT,
             typename OT = std::conditional_t<std::is_same_v<IT, SharableString>, SharableString, std::string>>
    static OT Sanitize(IT input);

private:
    ZHTML_API static std::string SanitizeWorker(const char* input);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename IT,
         typename OT/* = std::conditional_t<std::is_same_v<IT, SharableString>, SharableString, std::string>*/>
OT HtmlishSanitizer::Sanitize(IT input)
{
    const char* const input_data = SO::GetNullTerminatedString(input);

    if( strpbrk(input_data, "<>&\r\n") == nullptr )
        return OT(std::move(input));

    return SanitizeWorker(input_data);
}
