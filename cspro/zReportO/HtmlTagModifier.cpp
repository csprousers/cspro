#include "stdafx.h"
#include "HtmlTagModifier.h"
#include <zToolsO/RaiiHelpers.h>


std::string HtmlTagModifier::Process(const cs::string_sz html_input)
{
    std::string html_output;
    const RAII::SetValueAndRestoreOnDestruction html_modifier(m_html, &html_output);

    GumboOutput* const gumbo_output = gumbo_parse(html_input.c_str());
    ProcessNode(gumbo_output->root);
    gumbo_destroy_output(&kGumboDefaultOptions, gumbo_output);

    return html_output;
}


void HtmlTagModifier::ProcessNode(const GumboNode* const node)
{
    ASSERT(m_html != nullptr && node != nullptr);

    switch( node->type )
    {
        case GUMBO_NODE_DOCUMENT:
        case GUMBO_NODE_TEMPLATE:
        {
            throw ProgrammingErrorException();
        }

        case GUMBO_NODE_ELEMENT:
        {
            ProcessElement(node->v.element);
            break;
        }

        default:
        {
            const GumboText& text = node->v.text;
            ASSERT(text.original_text.length > 0);
            m_html->append(text.original_text.data, text.original_text.length);
            break;
        }
    }
}


void HtmlTagModifier::ProcessElement(const GumboElement& element)
{
    std::unique_ptr<std::string> end_tag;

    // because Gumbo parses the HTML as a full document, ignore tags that Gumbo inserts
    // that weren't in the original HTML
    if( element.original_tag.data != nullptr )
    {
        std::string start_tag(element.original_tag.data, element.original_tag.length);

        if( element.original_end_tag.length != 0 )
            end_tag = std::make_unique<std::string>(element.original_end_tag.data, element.original_end_tag.length);

        // potentially modify the start and end tags
        ProcessTag(start_tag, end_tag.get());

        // add the start tag
        m_html->append(start_tag);
    }

    // process children
    for( unsigned int i = 0; i < element.children.length; ++i )
        ProcessNode(static_cast<const GumboNode*>(element.children.data[i]));

    // add the end tag (when present)
    if( end_tag != nullptr )
        m_html->append(*end_tag);
}
