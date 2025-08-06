#include "StdAfx.h"
#include "CapiText.h"
#include <zMarkdown/Markdown.h>
#include <zEngineO/Nodes/TextTemplate.h>


EncodeType CapiText::GetEncodeType() const
{
    return ( m_format == Format::ReportMarkdown ) ? EncodeType::Markdown :
                                                    EncodeType::Html;
}


SharableString CapiText::GetHtml(const SharableString& text) const
{
    if( m_format == Format::ReportMarkdown )
        return Markdown::ToHtml(text.GetString());

    return text;
}


SharableString CapiText::GetHtmlWithEscapedTextTemplateDelimiters() const
{
    size_t first_tilde_pos;

    if( ( m_format == Format::ReportMarkdown ) &&
        ( ( first_tilde_pos = m_text->find('~') ) != std::string::npos ) )
    {
        std::string escaped_markdown = *m_text;
        SO::Replace(escaped_markdown, "~", "&#126;", first_tilde_pos);
        return GetHtml(std::move(escaped_markdown));
    }

    return GetHtml();
}


void CapiText::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::encoding, EncodeTypeStrings[static_cast<size_t>(GetEncodeType()) - 1])
               .Write(JK::text, m_text)
               .EndObject();
}


void CapiText::serialize(Serializer& ar)
{
    if( ar.PredatesVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ASSERT(m_format == Format::Html && m_programIndex == -1);
        ar & m_text.MakeModifiable();
        return;
    }

    ar.SerializeEnum(m_format);

    bool serializing_program_index;

    if( ar.IsSaving() )
        serializing_program_index = ( m_programIndex != -1 );

    ar & serializing_program_index;

    if( serializing_program_index )
    {
        ar & m_programIndex;
    }

    else
    {
        ar & m_text;
    }
}


DEFINE_ENUM_JSON_SERIALIZER_CLASS(CapiText::Format,
    { CapiText::Format::Html,           CapiText::FormatTexts[0] },
    { CapiText::Format::ReportHtml,     CapiText::FormatTexts[1] },
    { CapiText::Format::ReportMarkdown, CapiText::FormatTexts[2] })
