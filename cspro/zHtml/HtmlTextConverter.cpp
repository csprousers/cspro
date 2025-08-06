#include "stdafx.h"
#include "HtmlConverter.h"


std::string HtmlConverter::ToText(std::string html)
{
    // this is an incredibly crude implementation that...

    // strips excessive non-newline whitespace
    char* destination_itr = html.data();
    const char* start_source_itr = destination_itr;
    const char* const end_source_itr = start_source_itr + html.length();

    auto is_char_to_strip = [](const char ch)
    {
        return ( SO::IsWhitespaceChar(ch) && !is_crlf(ch) );
    };

    for( const char* source_itr = start_source_itr; source_itr != end_source_itr; ++source_itr )
    {
        if( !is_char_to_strip(*source_itr) || ( source_itr == start_source_itr || !is_char_to_strip(*( source_itr - 1 )) ) )
            *(destination_itr++) = *source_itr;
    }

    html.resize(destination_itr - start_source_itr);

    // converts line and paragraph breaks to newlines
    SO::Replace(html, "<br>", "\n");
    SO::Replace(html, "<br />", "\n");
    SO::Replace(html, "</p>", "\n");

    // strips other tags
    size_t search_pos = 0;

    while( true )
    {
        const auto [start_tag_pos, end_tag_pos] = SO::FindCharacters(html, '<', '>', search_pos);

        if( end_tag_pos == std::string::npos )
            break;

        html = html.substr(0, start_tag_pos) + html.substr(end_tag_pos + 1);

        search_pos = start_tag_pos;
    }

    // converts a few other character entities
    SO::Replace(html, "&nbsp;", " ");
    SO::Replace(html, "&lt;", "<");
    SO::Replace(html, "&gt;", ">");
    SO::Replace(html, "&amp;", "&");
    SO::Replace(html, "&#160;", " ");

    // get rid of \r characters
    SO::MakeNewlineLF(html);

    return html;
}
