#include "stdafx.h"
#include "ApplicationProperties.h"


CREATE_JSON_KEY(javaScript)


ApplicationProperties::ApplicationProperties()
    :   m_useHtmlComponentsInsteadOfNativeVersions(false)
{
}


bool ApplicationProperties::operator==(const ApplicationProperties& rhs) const
{
    return ( m_paradataProperties == rhs.m_paradataProperties &&
             m_mappingProperties == rhs.m_mappingProperties &&
             m_jsonProperties == rhs.m_jsonProperties &&
             m_javascriptProperties == rhs.m_javascriptProperties &&
             m_useHtmlComponentsInsteadOfNativeVersions == rhs.m_useHtmlComponentsInsteadOfNativeVersions &&
             UseHtmlDialogs == rhs.UseHtmlDialogs );
}


void ApplicationProperties::Open(const InterfaceString file_path, const bool silent/* = false*/, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(file_path, std::move(message_logger), [&]() { return ConvertPre80SpecFile(file_path); });

    try
    {
        json_reader->CheckVersion();
        json_reader->CheckFileType(JK::properties);

        CreateFromJsonWorker(*json_reader);
    }

    catch( const CSProException& exception )
    {
        json_reader->GetMessageLogger().RethrowException(file_path, exception);
    }

    // report any warnings
    json_reader->GetMessageLogger().DisplayWarnings(silent);
}


void ApplicationProperties::Save(const InterfaceString file_path) const
{
    const std::unique_ptr<JsonFileWriter> json_writer = JsonSpecFile::CreateWriter(file_path, JK::properties);

    WriteJson(*json_writer, false, true);

    json_writer->EndObject();
}


ApplicationProperties ApplicationProperties::CreateFromJson(const JsonNode& json_node)
{
    ApplicationProperties application_properties;
    application_properties.CreateFromJsonWorker(json_node);
    return application_properties;
}


void ApplicationProperties::CreateFromJsonWorker(const JsonNode& json_node)
{
    UseHtmlDialogs = json_node.GetOrDefault(JK::htmlDialogs, UseHtmlDialogs);

    if( json_node.Contains(JK::mapping) )
        m_mappingProperties = json_node.Get<MappingProperties>(JK::mapping);

    if( json_node.Contains(JK::paradata) )
        m_paradataProperties = json_node.Get<ParadataProperties>(JK::paradata);

    if( json_node.Contains(JK::json) )
        m_jsonProperties = json_node.Get<JsonProperties>(JK::json);

    if( json_node.Contains(JK::javaScript) )
        m_javascriptProperties = json_node.Get<JavaScriptProperties>(JK::javaScript);

    m_useHtmlComponentsInsteadOfNativeVersions = json_node.GetOrDefault(JK::useHtmlComponentsInsteadOfNativeVersions, m_useHtmlComponentsInsteadOfNativeVersions);
}


void ApplicationProperties::WriteJson(JsonWriter& json_writer, const bool write_to_new_json_object/* = true*/, const bool write_sections_if_all_default_values/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::htmlDialogs, UseHtmlDialogs);

    if( write_sections_if_all_default_values || m_mappingProperties != MappingProperties() )
        json_writer.Write(JK::mapping, m_mappingProperties);

    if( write_sections_if_all_default_values || m_paradataProperties != ParadataProperties() )
        json_writer.Write(JK::paradata, m_paradataProperties);

    if( write_sections_if_all_default_values || m_jsonProperties != JsonProperties() )
        json_writer.Write(JK::json, m_jsonProperties);

    if( write_sections_if_all_default_values || m_javascriptProperties != JavaScriptProperties() )
        json_writer.Write(JK::javaScript, m_javascriptProperties);

    json_writer.Write(JK::useHtmlComponentsInsteadOfNativeVersions, m_useHtmlComponentsInsteadOfNativeVersions);

    if( write_to_new_json_object )
        json_writer.EndObject();
}


void ApplicationProperties::serialize(Serializer& ar)
{
    ar & m_paradataProperties
       & m_mappingProperties;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        ar & m_jsonProperties;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
        ar & m_javascriptProperties;

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_3) )
        ar & m_useHtmlComponentsInsteadOfNativeVersions;
}
