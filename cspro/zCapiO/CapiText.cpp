#include "StdAfx.h"
#include "CapiText.h"
#include <zMarkdown/Markdown.h>


SharableString CapiText::GetHtml() const
{
    if( m_format == Format::ReportMarkdown )
        return Markdown::ToHtml(m_text.GetString());

    return m_text;
}


void CapiText::WriteJson(JsonWriter& json_writer) const
{
    json_writer.Write(m_text);
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
