#include "stdafx.h"
#include "Report.h"


// --------------------------------------------------------------------------
// Report
// --------------------------------------------------------------------------

Report::Report(std::string report_name, const ReportFile::EscapeType report_escape_type, std::string report_file_path)
    :   Symbol(std::move(report_name), SymbolType::Report),
        m_escapeType(report_escape_type),
        m_filePath(std::move(report_file_path)),
        m_programIndex(-1),
        m_reportTextBuilder(nullptr)
{
}


Report::Report(const ReportFile& report_file)
    :   Report(report_file.GetName(), report_file.GetEscapeType(), report_file.GetFilePath())
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

    ASSERT(!m_filePath.empty());

    const std::string name = PortableFunctions::PathGetFilename(m_filePath);
    const std::string extension = PortableFunctions::PathGetFileExtension(name);

    json_writer.BeginObject(JK::template_);

    json_writer.Write(JK::name, name)
               .Write(JK::escapeType, m_escapeType);

    if( PortableFunctions::FileIsRegular(m_filePath) )
    {
        json_writer.WritePath(JK::path, m_filePath);
    }

    else
    {
        json_writer.WriteNull(JK::path);
    }

    json_writer.Write(JK::extension, extension)
               .WriteIfHasValue(JK::contentType, MimeType::GetTypeFromFileExtension(extension));

    json_writer.EndObject();
}
