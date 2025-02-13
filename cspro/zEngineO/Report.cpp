#include "stdafx.h"
#include "Report.h"


// --------------------------------------------------------------------------
// Report
// --------------------------------------------------------------------------

Report::Report(std::wstring report_name, std::wstring report_filename)
    :   Symbol(std::move(report_name), SymbolType::Report),
        m_filename(std::move(report_filename)),
        m_isHtmlType(FileExtensions::IsFilenameHtml(m_filename)),
        m_programIndex(-1),
        m_reportTextBuilder(nullptr)
{
}


void Report::serialize_subclass(Serializer& ar)
{
    if( IsFunctionParameter() )
        return;

    ar & m_programIndex;
}


void Report::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    if( IsFunctionParameter() )
        return;

    ASSERT(!m_filename.empty());

    const std::wstring name = PortableFunctions::PathGetFilename(m_filename);
    const std::wstring extension = PortableFunctions::PathGetFileExtension(name);

    json_writer.BeginObject(JK::template_);

    if( PortableFunctions::FileIsRegular(m_filename) )
    {
        json_writer.WritePath(JK::path, m_filename);
    }

    else
    {
        json_writer.WriteNull(JK::path);
    }

    json_writer.Write(JK::name, name)
               .Write(JK::extension, extension)
               .WriteIfHasValue(JK::contentType, MimeType::GetTypeFromFileExtension(extension));

    json_writer.EndObject();
}
