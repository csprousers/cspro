#include "StdAfx.h"
#include "ParserCallback.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/Utf8.h>
#include <external/md4c/entity.h>


namespace
{
    // CSPro's encoders are a bit different from MD4C's, so if you
    // want to compare the output between the two, set this to true.
    constexpr bool MatchMd4cOutput = false;
}


Markdown::HtmlParserCallback::HtmlParserCallback()
    :   m_imageNestingLevel(0)
{
}


void Markdown::HtmlParserCallback::EnterBlock(MD_BLOCKTYPE type, void* const detail)
{
    switch( type )
    {
        case MD_BLOCK_DOC:
            break;

        case MD_BLOCK_QUOTE:
            m_html.append("<blockquote>\n");
            break;

        case MD_BLOCK_UL:
            m_html.append("<ul>\n");
            break;

        case MD_BLOCK_OL:
            render_open_ol_block(static_cast<const MD_BLOCK_OL_DETAIL*>(detail));
            break;

        case MD_BLOCK_LI:
            render_open_li_block(static_cast<const MD_BLOCK_LI_DETAIL*>(detail));
            break;

        case MD_BLOCK_HR:
            m_html.append("<hr>\n");
            break;

        case MD_BLOCK_H:
            m_html.append("<h").append(IntToString(static_cast<const MD_BLOCK_H_DETAIL*>(detail)->level)).append(">");
            break;

        case MD_BLOCK_CODE:
            render_open_code_block(static_cast<const MD_BLOCK_CODE_DETAIL*>(detail));
            break;

        case MD_BLOCK_HTML:
            break;

        case MD_BLOCK_P:
            m_html.append("<p>");
            break;

        case MD_BLOCK_TABLE:
            m_html.append("<table>\n");
            break;

        case MD_BLOCK_THEAD:
            m_html.append("<thead>\n");
            break;

        case MD_BLOCK_TBODY:
            m_html.append("<tbody>\n");
            break;

        case MD_BLOCK_TR:
            m_html.append("<tr>\n");
            break;

        case MD_BLOCK_TH:
            render_open_td_block("th", static_cast<const MD_BLOCK_TD_DETAIL*>(detail));
            break;

        case MD_BLOCK_TD:
            render_open_td_block("td", static_cast<const MD_BLOCK_TD_DETAIL*>(detail));
            break;

        default:
            ASSERT(false);
    }
}


void Markdown::HtmlParserCallback::LeaveBlock(const MD_BLOCKTYPE type, void* const detail)
{
    switch( type )
    {
        case MD_BLOCK_DOC:
            break;

        case MD_BLOCK_QUOTE:
            m_html.append("</blockquote>\n");
            break;

        case MD_BLOCK_UL:
            m_html.append("</ul>\n");
            break;

        case MD_BLOCK_OL:
            m_html.append("</ol>\n");
            break;

        case MD_BLOCK_LI:
            m_html.append("</li>\n");
            break;

        case MD_BLOCK_HR:
            break;

        case MD_BLOCK_H:
            m_html.append("</h").append(IntToString(static_cast<const MD_BLOCK_H_DETAIL*>(detail)->level)).append(">\n");
            break;

        case MD_BLOCK_CODE:
            m_html.append("</code></pre>\n");
            break;

        case MD_BLOCK_HTML:
            break;

        case MD_BLOCK_P:
            m_html.append("</p>\n");
            break;

        case MD_BLOCK_TABLE:
            m_html.append("</table>\n");
            break;

        case MD_BLOCK_THEAD:
            m_html.append("</thead>\n");
            break;

        case MD_BLOCK_TBODY:
            m_html.append("</tbody>\n");
            break;

        case MD_BLOCK_TR:
            m_html.append("</tr>\n");
            break;

        case MD_BLOCK_TH:
            m_html.append("</th>\n");
            break;

        case MD_BLOCK_TD:
            m_html.append("</td>\n");
            break;

        default:
            ASSERT(false);
    }
}


void Markdown::HtmlParserCallback::EnterSpan(const MD_SPANTYPE type, void* const detail)
{
    const bool inside_img = ( m_imageNestingLevel > 0 );

    if( type == MD_SPAN_IMG )
        ++m_imageNestingLevel;

    if( !inside_img )
        EnterSpanWorker(type, detail);
}


void Markdown::HtmlParserCallback::EnterSpanWorker(const MD_SPANTYPE type, void* const detail)
{
    switch( type)
    {
        case MD_SPAN_EM:
            m_html.append("<em>");
            break;

        case MD_SPAN_STRONG:
            m_html.append("<strong>");
            break;

        case MD_SPAN_U:
            m_html.append("<u>");
            break;

        case MD_SPAN_A:
            render_open_a_span(static_cast<const MD_SPAN_A_DETAIL*>(detail));
            break;

        case MD_SPAN_IMG:
            render_open_img_span(static_cast<const MD_SPAN_IMG_DETAIL*>(detail));
            break;

        case MD_SPAN_CODE:
            m_html.append("<code>");
            break;

        case MD_SPAN_DEL:
            m_html.append("<del>");
            break;

        case MD_SPAN_LATEXMATH:
            m_html.append("<x-equation>");
            break;

        case MD_SPAN_LATEXMATH_DISPLAY:
            m_html.append("<x-equation type=\"display\">");
            break;

        case MD_SPAN_WIKILINK:
            render_open_wikilink_span(static_cast<const MD_SPAN_WIKILINK_DETAIL*>(detail));
            break;

        default:
            ASSERT(false);
    }
}


void Markdown::HtmlParserCallback::LeaveSpan(const MD_SPANTYPE type, void* const detail)
{
    if( type == MD_SPAN_IMG )
        --m_imageNestingLevel;

    if( m_imageNestingLevel == 0 )
        LeaveSpanWorker(type, detail);
}


void Markdown::HtmlParserCallback::LeaveSpanWorker(const MD_SPANTYPE type, void* const detail)
{
    switch( type )
    {
        case MD_SPAN_EM:
            m_html.append("</em>");
            break;

        case MD_SPAN_STRONG:
            m_html.append("</strong>");
            break;

        case MD_SPAN_U:
            m_html.append("</u>");
            break;

        case MD_SPAN_A:
            m_html.append("</a>");
            break;

        case MD_SPAN_IMG:
            render_close_img_span(static_cast<const MD_SPAN_IMG_DETAIL*>(detail));
            break;

        case MD_SPAN_CODE:
            m_html.append("</code>");
            break;

        case MD_SPAN_DEL:
            m_html.append("</del>");
            break;

        case MD_SPAN_LATEXMATH:
        case MD_SPAN_LATEXMATH_DISPLAY:
            m_html.append("</x-equation>");
            break;

        case MD_SPAN_WIKILINK:
            m_html.append("</x-wikilink>");
            break;

        default:
            ASSERT(false);
    }
}


void Markdown::HtmlParserCallback::ProcessOutput(const MD_TEXTTYPE type, const std::string_view text_sv)
{
    switch( type )
    {
        case MD_TEXT_NULLCHAR:
            render_utf8_codepoint(0x0000, EscapeType::Verbatim);
            break;

        case MD_TEXT_BR:
            m_html.append(( m_imageNestingLevel == 0 ) ? "<br>\n" : " ");
            break;

        case MD_TEXT_SOFTBR:
            m_html.append(( m_imageNestingLevel == 0 ) ? "\n" : " ");
            break;

        case MD_TEXT_ENTITY:
            render_entity(text_sv, EscapeType::ForHtmlOrTag);
            break;

        case MD_TEXT_CODE:
            ProcessOutputCode(text_sv);
            break;

        case MD_TEXT_HTML:
            m_html.append(text_sv);
            break;

        default:
            render(text_sv, EscapeType::ForHtmlOrTag);
            break;
    }
}


void Markdown::HtmlParserCallback::ProcessOutputCode(const std::string_view text_sv)
{
    render(text_sv, EscapeType::ForHtmlOrTag);
}


void Markdown::HtmlParserCallback::render(const std::string_view text_sv, const EscapeType escape_type)
{
    switch( escape_type )
    {
        case EscapeType::Verbatim:
        {
            m_html.append(text_sv);
            break;
        }

        case EscapeType::ForHtmlOrTag:
        {
            m_html.append(Encoders::ToHtmlTagValue(text_sv));
            break;
        }

        default:
        {
            ASSERT(escape_type == EscapeType::ForUrl);
            std::string html = Encoders::ToUri(text_sv);

            if( MatchMd4cOutput )
            {
                SO::Replace(html, "&", "&amp;");

                // percent-encoded characters are rendered in uppercase
                for( size_t percent_pos = 0; ( percent_pos = html.find('%', percent_pos) ) != std::string::npos; percent_pos += 3 )
                    html.replace(percent_pos, 3, SO::ToUpper(html.substr(percent_pos, 3)));
            }

            m_html.append(html);
        }
    }
}


void Markdown::HtmlParserCallback::render_utf8_codepoint(const unsigned codepoint, const EscapeType escape_type)
{
    constexpr const char Utf8ReplacementChar[] = { static_cast<char>(0xef),
                                                   static_cast<char>(0xbf),
                                                   static_cast<char>(0xbd) };

    constexpr unsigned MaxValidCodepoint = 0x10ffff;

    if( codepoint > 0 && codepoint <= MaxValidCodepoint )
    {
        render(TC::GetUtf8ForWideChar(static_cast<wchar_t>(codepoint)), escape_type);
    }

    else
    {
        render(std::string_view(Utf8ReplacementChar, _countof(Utf8ReplacementChar)), escape_type);
    }
}


void Markdown::HtmlParserCallback::render_entity(const std::string_view text_sv, const EscapeType escape_type)
{
    if( text_sv.size() > 3 && text_sv[1] == '#' )
    {
        unsigned codepoint;

        if( text_sv[2] == 'x' || text_sv[2] == 'X' )
        {
            // hexadecimal entity (e.g. "&#x1234abcd;")
            codepoint = Encoders::ToHexValue<unsigned>(text_sv.substr(3, text_sv.size() - 4));
        }

        else
        {
            // decimal entity (e.g. "&#1234;")
            codepoint = 0;

            for( const char ch : text_sv.substr(2, text_sv.size() - 3) )
                codepoint = 10 * codepoint + ( ch - '0' );
        }

        render_utf8_codepoint(codepoint, escape_type);
        return;

    }

    else
    {
        // named entity (e.g. "&nbsp;")
        const ENTITY* const entity = entity_lookup(text_sv.data(), text_sv.size());

        if( entity != nullptr )
        {
            render_utf8_codepoint(entity->codepoints[0], escape_type);

            if( entity->codepoints[1] != 0 )
                render_utf8_codepoint(entity->codepoints[1], escape_type);

            return;
        }
    }

    render(text_sv, escape_type);
}


void Markdown::HtmlParserCallback::render_attribute(const MD_ATTRIBUTE* const attr, const EscapeType escape_type)
{
    for( int i = 0; attr->substr_offsets[i] < attr->size; ++i )
    {
        const MD_TEXTTYPE type = attr->substr_types[i];
        const MD_OFFSET off = attr->substr_offsets[i];
        const std::string_view text_sv(attr->text + off,
                                       attr->substr_offsets[i + 1] - off);

        switch( type )
        {
            case MD_TEXT_NULLCHAR:
                render_utf8_codepoint(0x0000, EscapeType::Verbatim);
                break;

            case MD_TEXT_ENTITY:
                render_entity(text_sv, escape_type);
                break;

            default:
                render(text_sv, escape_type);
                break;
        }
    }
}


void Markdown::HtmlParserCallback::render_open_ol_block(const MD_BLOCK_OL_DETAIL* const det)
{
    ( det->start == 1 ) ? m_html.append("<ol>\n") :
                          m_html.append("<ol start=\"").append(IntToString(det->start)).append("\">\n");
}


void Markdown::HtmlParserCallback::render_open_li_block(const MD_BLOCK_LI_DETAIL* const det)
{
    if( det->is_task )
    {
        m_html.append("<li class=\"task-list-item\">"
                      "<input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled");

        if( det->task_mark == 'x' || det->task_mark == 'X' )
            m_html.append(" checked");

        m_html.append(">");
    }

    else
    {
        m_html.append("<li>");
    }
}


void Markdown::HtmlParserCallback::render_open_code_block(const MD_BLOCK_CODE_DETAIL* const det)
{
    m_html.append("<pre><code");

    if( det->lang.text != nullptr )
    {
        m_html.append(" class=\"language-");
        render_attribute(&det->lang, EscapeType::ForHtmlOrTag);
        m_html.append("\"");
    }

    m_html.append(">");
}


void Markdown::HtmlParserCallback::render_open_td_block(const MD_CHAR* const cell_type, const MD_BLOCK_TD_DETAIL* const det)
{
    m_html.append("<");
    m_html.append(cell_type);

    switch( det->align )
    {
        case MD_ALIGN_LEFT:
            m_html.append(" align=\"left\">");
            break;

        case MD_ALIGN_CENTER:
            m_html.append(" align=\"center\">");
            break;

        case MD_ALIGN_RIGHT:
            m_html.append(" align=\"right\">");
            break;

        default:
            m_html.append(">");
            break;
    }
}


void Markdown::HtmlParserCallback::render_open_a_span(const MD_SPAN_A_DETAIL* const det)
{
    m_html.append("<a href=\"");
    render_attribute(&det->href, EscapeType::ForUrl);

    if( det->title.text != nullptr )
    {
        m_html.append("\" title=\"");
        render_attribute(&det->title, EscapeType::ForHtmlOrTag);
    }

    m_html.append("\">");
}


void Markdown::HtmlParserCallback::render_open_img_span(const MD_SPAN_IMG_DETAIL* const det)
{
    m_html.append("<img src=\"");
    render_attribute(&det->src, EscapeType::ForUrl);

    m_html.append("\" alt=\"");
}


void Markdown::HtmlParserCallback::render_close_img_span(const MD_SPAN_IMG_DETAIL* const det)
{
    if( det->title.text != nullptr )
    {
        m_html.append("\" title=\"");
        render_attribute(&det->title, EscapeType::ForHtmlOrTag);
    }

    m_html.append("\">");
}


void Markdown::HtmlParserCallback::render_open_wikilink_span(const MD_SPAN_WIKILINK_DETAIL* const det)
{
    m_html.append("<x-wikilink data-target=\"");
    render_attribute(&det->target, EscapeType::ForHtmlOrTag);
    m_html.append("\">");
}
