#pragma once

#include <zHtml/zHtml.h>
#include <zToolsO/Encoders.h>


namespace HtmlTextConverter
{
    ZHTML_API std::wstring HtmlToText(std::wstring html);

    inline std::wstring TextToHtml(wstring_view text, bool escape_spaces = true)
    {
        return Encoders::ToHtml(text, escape_spaces);
    }
}
