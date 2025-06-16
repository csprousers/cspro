#include "stdafx.h"
#include "Report.h"


// --------------------------------------------------------------------------
// Report
// --------------------------------------------------------------------------

Report::Report(std::string report_name, const ReportFile::Encoding report_encoding, std::string report_file_path)
    :   Symbol(std::move(report_name), SymbolType::Report),
        m_encoding(report_encoding),
        m_filePath(std::move(report_file_path)),
        m_programIndex(-1),
        m_reportTextBuilder(nullptr)
{
}


Report::Report(const ReportFile& report_file)
    :   Report(report_file.GetName(), report_file.GetEncoding(), report_file.GetFilePath())
{
}


std::unique_ptr<Report> Report::CreateReportFunctionParamter(std::string report_name)
{
    return std::unique_ptr<Report>(new Report(std::move(report_name), ReportFile::Encoding::None, std::string()));
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
               .Write(JK::encoding, m_encoding);

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


void Report::WriteValueToJson(JsonWriter& json_writer) const
{
    if( m_reportTextBuilder != nullptr )
    {
        json_writer.WriteEngineValue(*m_reportTextBuilder);
    }

    else
    {
        json_writer.WriteNull();
    }
}


void Report::SetValueFromJson(const JsonNode& json_node)
{
    if( m_reportTextBuilder == nullptr )
        throw CSProException("The report creation has not yet been initiated.");

    *m_reportTextBuilder = json_node.GetEngineValue<std::string>();
}
