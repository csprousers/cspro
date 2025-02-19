#include "StdAfx.h"
#include "BasicLogger.h"


namespace
{
    constexpr const char* ColorNames[] =
    {
        "Black",
        "Red",
        "DarkBlue",
        "SlateBlue"
    };
}


std::string BasicLogger::ToString() const
{
    std::string text;

    for( const Span& span : m_spans )
        text.append(span.text);

    return text;
}


std::string BasicLogger::ToHtml() const
{
    std::string html = "<html><body><p style=\"word-wrap:break-word; margin:0px; padding:0px; border:0px; "
                       "background-color:#ffffff; font-family: Consolas, monaco, monospace; font-size:10pt;\">";

    std::optional<Color> last_color;

    auto end_color_span = [&]()
    {
        if( last_color.has_value() )
            html.append("</span>");
    };

    for( const Span& span : m_spans )
    {
        // change the color if necessary
        if( last_color != span.color )
        {
            end_color_span();
            html.append(FormatText("<span style=\"color: %s;\">", ColorNames[static_cast<size_t>(span.color)]));
            last_color = span.color;
        }

        html.append(Encoders::ToHtml(span.text));
    }

    end_color_span();

    html.append("</p></body></html>");

    return html;
}
