#include "StdAfx.h"
#include "BasicLogger.h"
#include <zToolsO/Encoders.h>


namespace
{
    const TCHAR* const ColorNames[] =
    {
        _T("Black"),
        _T("Red"),
        _T("DarkBlue"),
        _T("SlateBlue")
    };
}


std::wstring BasicLogger::ToString() const
{
    std::wstring text;

    for( const Span& span : m_spans )
        text.append(span.text);

    return text;
}


std::wstring BasicLogger::ToHtml() const
{
    std::wstring html = _T("<html><body><p style=\"word-wrap:break-word; margin:0px; padding:0px; border:0px; ")
                        _T("background-color:#ffffff; font-family: Consolas, monaco, monospace; font-size:10pt;\">");

    std::optional<Color> last_color;

    auto end_color_span = [&]()
    {
        if( last_color.has_value() )
            html.append(_T("</span>"));
    };

    for( const Span& span : m_spans )
    {
        // change the color if necessary
        if( last_color != span.color )
        {
            end_color_span();
            SO::AppendFormat(html, _T("<span style=\"color: %s;\">"), ColorNames[(size_t)span.color]);
            last_color = span.color;
        }

        html.append(Encoders::ToHtml(span.text));
    }

    end_color_span();

    html.append(_T("</p></body></html>"));

    return html;
}
