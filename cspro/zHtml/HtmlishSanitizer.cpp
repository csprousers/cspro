#include "stdafx.h"
#include "HtmlishSanitizer.h"
#include <external/gumbo/gumbo.h>
#include <external/md4c/entity.h>


// --------------------------------------------------------------------------
// HtmlishSanitizer::Worker
// --------------------------------------------------------------------------

class HtmlishSanitizer::Worker
{
public:
    Worker(cs::string_sz input, std::string& html);
    ~Worker();

    void Sanitize();

private:
    void ProcessNode(const GumboNode* node);
    void ProcessElement(const GumboElement& element);

    void AppendText(std::string_view text_sv);
    void AppendTextRequiringEscaping(std::string_view text_sv);

    // Returns the length of the entity if valid, or 0.
    size_t IsHtmlEntity(std::string_view text_sv);

private:
    GumboOutput* m_gumboOutput;
    std::string& m_html;
};


HtmlishSanitizer::Worker::Worker(const cs::string_sz input, std::string& html)
    :   m_gumboOutput(gumbo_parse(input.c_str())),
        m_html(html)
{
    ASSERT(m_gumboOutput != nullptr && m_html.empty());
}


HtmlishSanitizer::Worker::~Worker()
{
    gumbo_destroy_output(&kGumboDefaultOptions, m_gumboOutput);
}


void HtmlishSanitizer::Worker::Sanitize()
{
    ProcessNode(m_gumboOutput->root);
}


void HtmlishSanitizer::Worker::ProcessNode(const GumboNode* const node)
{
    ASSERT(node != nullptr);

    switch( node->type )
    {
        case GUMBO_NODE_DOCUMENT:
        case GUMBO_NODE_TEMPLATE:
        {
            ASSERT(false);
            break;
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
            AppendTextRequiringEscaping(std::string_view(text.original_text.data, text.original_text.length));
            break;
        }
    }
}


void HtmlishSanitizer::Worker::ProcessElement(const GumboElement& element)
{
    // because Gumbo parses the text as a full document, ignore tags that Gumbo inserts
    // that weren't in the original text
    const bool element_exists_in_input = ( element.original_tag.data != nullptr );

    ASSERT(element_exists_in_input || ( element.original_end_tag.length == 0 &&
                                        element.original_tag.length == 0 ));

    // valid tags will be output directly, but invalid tags will be assumed
    // to be literal output and will be escaped for HTML
    const bool valid_tag = ( element.tag != GUMBO_TAG_UNKNOWN );

    if( element_exists_in_input )
    {
        const std::string_view start_tag_sv(element.original_tag.data, element.original_tag.length);
        valid_tag ? AppendText(start_tag_sv) :
                    AppendTextRequiringEscaping(start_tag_sv);
    }

    // process children
    for( unsigned int i = 0; i < element.children.length; ++i )
        ProcessNode(static_cast<const GumboNode*>(element.children.data[i]));

    if( element.original_end_tag.length != 0 )
    {
        const std::string_view end_tag_sv(element.original_end_tag.data, element.original_end_tag.length);
        valid_tag ? AppendText(end_tag_sv) :
                    AppendTextRequiringEscaping(end_tag_sv);
    }
}


void HtmlishSanitizer::Worker::AppendText(const std::string_view text_sv)
{
    m_html.append(text_sv.data(), text_sv.length());
}


void HtmlishSanitizer::Worker::AppendTextRequiringEscaping(const std::string_view text_sv)
{
    // process newlines and HTML entities (starting in &), but otherwise use Encoders::ToHtml
    const char* segment_start = text_sv.data();
    const char* segment_itr = segment_start;
    const char* const text_end = segment_start + text_sv.length();

    auto output_and_start_new_segment = [&](const size_t entity_length)
    {
        if( segment_start != segment_itr )
            m_html.append(Encoders::ToHtml(std::string_view(segment_start, segment_itr - segment_start), false));

        segment_itr += entity_length;
        segment_start = segment_itr;
    };

    for( ; segment_itr != text_end; )
    {
        const char ch = *segment_itr;

        // write newlines as <br>
        if( ch == '\n' )
        {
            output_and_start_new_segment(1);
            m_html.append("<br>");
        }

        // ignore carriage returns
        else if( ch == '\r' )
        {
            output_and_start_new_segment(1);
        }

        // determine if & is part of an HTML entity
        else if( ch == '&' )
        {
            const size_t entity_length = IsHtmlEntity(std::string_view(segment_itr, text_end - segment_itr));

            if( entity_length != 0 )
            {
                const char* const entity_segment_start = segment_itr;
                output_and_start_new_segment(entity_length);
                m_html.append(entity_segment_start, entity_length);
            }

            else
            {
                ++segment_itr;
            }
        }

        else
        {
            ++segment_itr;
        }
    }

    if( segment_start != text_end )
        m_html.append(Encoders::ToHtml(std::string_view(segment_start, text_end - segment_start), false));
}


size_t HtmlishSanitizer::Worker::IsHtmlEntity(const std::string_view text_sv)
{
    const size_t semicolon_pos = text_sv.find(';');

    if( semicolon_pos == std::string_view::npos )
        return 0;

    const std::string_view entity_sv = text_sv.substr(0, semicolon_pos + 1);

    // allow hexadecimal and decimal entities
    if( entity_sv.size() > 3 && entity_sv[1] == '#' )
    {
        auto is_valid = [&](const std::string_view numeric_value_sv, const char* const valid_chars)
        {
            for( const char ch : numeric_value_sv )
            {
                if( strchr(valid_chars, std::tolower(ch)) == nullptr )
                    return false;
            }

            return true;
        };

        // check for a hexadecimal entity (e.g. "&#x1234abcd;")
        if( entity_sv[2] == 'x' || entity_sv[2] == 'X' )
        {
            if( is_valid(entity_sv.substr(3, entity_sv.length() - 4), Encoders::HexChars) )
                return entity_sv.length();
        }

        // check for a decimal entity (e.g. "&#1234;")
        else if( is_valid(entity_sv.substr(2, entity_sv.length() - 3), Encoders::DecimalChars) )
        {
            return entity_sv.length();
        }
    }

    // check for a named entity
    if( entity_lookup(entity_sv.data(), entity_sv.size()) != nullptr )
        return entity_sv.length();

    return 0;
}



// --------------------------------------------------------------------------
// HtmlishSanitizer
// --------------------------------------------------------------------------

std::string HtmlishSanitizer::SanitizeWorker(const char* const input)
{
    std::string html;
    Worker worker(input, html);

    worker.Sanitize();

    return html;
}
