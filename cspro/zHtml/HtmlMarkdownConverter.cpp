#include "stdafx.h"
#include "HtmlConverter.h"
#include <zMarkdown/Markdown.h>
#include <external/gumbo/gumbo.h>


// --------------------------------------------------------------------------
// HtmlToMarkdownConverter
// --------------------------------------------------------------------------

class HtmlToMarkdownConverter
{
public:
    HtmlToMarkdownConverter(std::string html);
    ~HtmlToMarkdownConverter();

    std::string Convert() const;

private:
    std::string ProcessNode(const GumboNode* node) const;
    std::string ProcessElement(const GumboElement& element) const;
    std::string ProcessChildren(const GumboElement& element, bool combine_children_into_single_line) const;

    std::string CreateMarkdown(const GumboElement& element, bool combine_children_into_single_line,
                               std::string start_markdown, std::string_view end_markdown_sv) const;
    std::string CreateMarkdownLink(const GumboElement& element) const;
    static std::string CreateMarkdownImage(const GumboElement& element);
    std::string CreateMarkdownList(const GumboElement& element) const;

    std::string GetEscapedHtmlTextForMarkdown(std::string_view text_sv) const;

private:
    std::string m_html;
    GumboOutput* m_gumboOutput;
    const std::string m_nonHtmlEscapeChars;
};


HtmlToMarkdownConverter::HtmlToMarkdownConverter(std::string html)
    :   m_html(std::move(html)),
        m_gumboOutput(gumbo_parse(m_html.c_str())),
        m_nonHtmlEscapeChars(Encoders::GetMarkdownNonHtmlEscapeChars())
{
    ASSERT(m_html.find('\r') == std::string::npos);
    ASSERT(m_gumboOutput != nullptr);
}


HtmlToMarkdownConverter::~HtmlToMarkdownConverter()
{
    gumbo_destroy_output(&kGumboDefaultOptions, m_gumboOutput);
}


std::string HtmlToMarkdownConverter::Convert() const
{
    std::string markdown = ProcessNode(m_gumboOutput->root);

    // period (dot) characters are quite common and will be escaped to: \.
    // to make the output more readable, convert these back to .
    bool last_ch_was_escape = false;

    for( auto itr = markdown.begin(); itr != markdown.end(); ++itr )
    {
        if( last_ch_was_escape )
        {
            last_ch_was_escape = false;

            if( *itr == '.' )
            {
                // do not convert a dot that follows a number as that could be interpreted as an ordered list;
                // e.g.,: <p>2025. This is a year, not an ordered list!</p>
                const auto& two_chars_previous_itr = itr - 2;

                if( two_chars_previous_itr < markdown.begin() ||
                    !is_digit(*two_chars_previous_itr) )
                {
                    itr = markdown.erase(itr - 1);
                }
            }
        }

        else if( *itr == '\\' )
        {
            last_ch_was_escape = true;
        }
    }

    return markdown;
}


std::string HtmlToMarkdownConverter::ProcessNode(const GumboNode* const node) const
{
    ASSERT(node != nullptr);

    switch( node->type )
    {
        case GUMBO_NODE_DOCUMENT:
        case GUMBO_NODE_TEMPLATE:
        {
            return ReturnProgrammingError(std::string());
        }

        case GUMBO_NODE_ELEMENT:
        {
            return ProcessElement(node->v.element);
        }

        default:
        {
            const GumboText& text = node->v.text;
            ASSERT(text.original_text.length > 0);
            return GetEscapedHtmlTextForMarkdown(std::string_view(text.original_text.data, text.original_text.length));
        }
    }
}


std::string HtmlToMarkdownConverter::ProcessElement(const GumboElement& element) const
{
    // because Gumbo parses the text as a full document, ignore tags that Gumbo inserts
    // that weren't in the original text
    const bool element_exists_in_input = ( element.original_tag.data != nullptr );

    ASSERT(element_exists_in_input || ( element.original_end_tag.length == 0 &&
                                        element.original_tag.length == 0 ));

    switch( element.tag )
    {
        case GUMBO_TAG_H1:
        case GUMBO_TAG_H2:
        case GUMBO_TAG_H3:
        case GUMBO_TAG_H4:
        case GUMBO_TAG_H5:
        case GUMBO_TAG_H6:
        {
            static_assert(GUMBO_TAG_H1 + 5 == GUMBO_TAG_H6);
            return CreateMarkdown(element, true, "###### " + ( GUMBO_TAG_H6 - element.tag ), "\n\n");
        }

        case GUMBO_TAG_P:
            return CreateMarkdown(element, false, std::string(), "\n\n");

        case GUMBO_TAG_BR:
            ASSERT(element.children.length == 0);
            return "  \n";

        case GUMBO_TAG_HR:
            ASSERT(element.children.length == 0);
            return "\n---\n";

        case GUMBO_TAG_STRONG:
        case GUMBO_TAG_B:
            return CreateMarkdown(element, true, "**", "**");

        case GUMBO_TAG_EM:
        case GUMBO_TAG_I:
            return CreateMarkdown(element, true, "*", "*");

        case GUMBO_TAG_A:
            return CreateMarkdownLink(element);

        case GUMBO_TAG_IMG:
            return CreateMarkdownImage(element);

        case GUMBO_TAG_UL:
        case GUMBO_TAG_OL:
            return CreateMarkdownList(element);
    }

    // if this is a tag that we do not process, write out the HTML tags only when the tag existed in the input
    if( element_exists_in_input )
    {
        const std::string_view end_tag_sv = ( element.original_end_tag.length != 0 ) ?
            std::string_view(element.original_end_tag.data, element.original_end_tag.length) :
            std::string_view();

        return CreateMarkdown(element, false,
                              std::string(element.original_tag.data, element.original_tag.length),
                              end_tag_sv);
    }

    else
    {
        return ProcessChildren(element, false);
    }
}


std::string HtmlToMarkdownConverter::ProcessChildren(const GumboElement& element, const bool combine_children_into_single_line) const
{
    std::string markdown;

    // add the content in the child nodes
    for( unsigned int i = 0; i < element.children.length; ++i )
        markdown.append(ProcessNode(static_cast<const GumboNode*>(element.children.data[i])));

    // potentially turn newlines into br tags so that Markdown can properly process inputs such as:
    // <b>Beginning of bold...
    //
    // ...end of bold</b>
    if( combine_children_into_single_line )
    {
        SO::MakeTrim(markdown);
        SO::Replace(markdown, "\n", "<br>");
    }

    return markdown;
}


std::string HtmlToMarkdownConverter::CreateMarkdown(const GumboElement& element, const bool combine_children_into_single_line,
                                                    std::string start_markdown, const std::string_view end_markdown_sv) const
{
    return start_markdown.append(ProcessChildren(element, combine_children_into_single_line))
                         .append(end_markdown_sv);
}


std::string HtmlToMarkdownConverter::CreateMarkdownLink(const GumboElement& element) const
{
    GumboAttribute* const href = gumbo_get_attribute(&element.attributes, "href");

    if( href == nullptr )
        return ProcessChildren(element, false);

    return SO::Concatenate("[", ProcessChildren(element, true), "](", CreateMarkdownUrl(href->value), ")");
}


std::string HtmlToMarkdownConverter::CreateMarkdownImage(const GumboElement& element)
{
    GumboAttribute* const src = gumbo_get_attribute(&element.attributes, "src");

    if( src == nullptr )
        return std::string();

    GumboAttribute* const alt = gumbo_get_attribute(&element.attributes, "alt");

    const std::string alt_text = ( alt != nullptr ) ? Encoders::ToMarkdown(alt->value) :
                                                      std::string();

    return SO::Concatenate("![", alt_text, "](", CreateMarkdownUrl(src->value), ")");
}


std::string HtmlToMarkdownConverter::CreateMarkdownList(const GumboElement& element) const
{
    const bool ordered = ( element.tag == GUMBO_TAG_OL );
    std::string markdown;

    int li_counter = 0;

    for( unsigned int i = 0; i < element.children.length; ++i )
    {
        const GumboNode* const child_element = static_cast<const GumboNode*>(element.children.data[i]);
        ASSERT(child_element != nullptr);

        if( child_element->type == GUMBO_NODE_ELEMENT && child_element->v.element.tag == GUMBO_TAG_LI )
        {
            if( ordered )
            {
                markdown.append(IntToString(++li_counter))
                        .append(". ");
            }

            else
            {
                markdown.append("- ");
            }

            markdown.append(ProcessChildren(child_element->v.element, true))
                    .push_back('\n');
        }
    }

    markdown.push_back('\n');

    return markdown;
}


std::string HtmlToMarkdownConverter::GetEscapedHtmlTextForMarkdown(const std::string_view text_sv) const
{
    std::string markdown;

    // only escape Markdown-specific characters
    for( const char ch : text_sv )
    {
        if( m_nonHtmlEscapeChars.find(ch) != std::string::npos )
            markdown.push_back('\\');

        markdown.push_back(ch);
    }

    return markdown;
}



// --------------------------------------------------------------------------
// HtmlConverter
// --------------------------------------------------------------------------

std::string HtmlConverter::ToMarkdown(std::string html)
{
    const HtmlToMarkdownConverter converter(SO::ToNewlineLF(std::move(html)));
    return converter.Convert();
}
