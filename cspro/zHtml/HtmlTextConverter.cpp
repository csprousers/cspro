#include "stdafx.h"
#include "HtmlTextConverter.h"


std::wstring HtmlTextConverter::HtmlToText(std::wstring html)
{
    // this is an incredibly crude implementation that...

    // strips excessive whitespace
    const TCHAR* start_source_itr = html.data();
    const TCHAR* const end_source_itr = start_source_itr + html.length();
    TCHAR* destination_itr = const_cast<TCHAR*>(start_source_itr);

    for( const TCHAR* source_itr = start_source_itr; source_itr != end_source_itr; ++source_itr )
    {
        if( !std::iswspace(*source_itr) || ( source_itr == start_source_itr || !std::iswspace(*( source_itr - 1 )) ) )
            *(destination_itr++) = *source_itr;
    }

    html.resize(destination_itr - start_source_itr);

    // converts line and paragraph breaks to newlines
    SO::Replace(html, _T("<br>"), _T("\n"));
    SO::Replace(html, _T("<br />"), _T("\n"));
    SO::Replace(html, _T("</p>"), _T("\n"));

    // strips other tags
    size_t search_pos = 0;

    while( true )
    {
        const auto [start_tag_pos, end_tag_pos] = SO::FindCharacters(html, '<', '>', search_pos);

        if( end_tag_pos == std::wstring::npos )
            break;

        html = html.substr(0, start_tag_pos) + html.substr(end_tag_pos + 1);

        search_pos = start_tag_pos;
    }

    // converts a few other character entities
    SO::Replace(html, _T("&nbsp;"), _T(" "));
    SO::Replace(html, _T("&lt;"), _T("<"));
    SO::Replace(html, _T("&gt;"), _T(">"));
    SO::Replace(html, _T("&amp;"), _T("&"));
    SO::Replace(html, _T("&#160;"), _T(" "));

    // get rid of \r characters
    SO::MakeNewlineLF(html);

    return html;
}
