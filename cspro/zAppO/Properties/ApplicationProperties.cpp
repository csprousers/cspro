#include "stdafx.h"
#include "ApplicationProperties.h"


ApplicationProperties::ApplicationProperties()
    :   m_useHtmlComponentsInsteadOfNativeVersions(false)
{
}


bool ApplicationProperties::operator==(const ApplicationProperties& rhs) const
{
    return ( m_paradataProperties == rhs.m_paradataProperties &&
             m_mappingProperties == rhs.m_mappingProperties &&
             m_jsonProperties == rhs.m_jsonProperties &&
             m_useHtmlComponentsInsteadOfNativeVersions == rhs.m_useHtmlComponentsInsteadOfNativeVersions &&
             UseHtmlDialogs == rhs.UseHtmlDialogs );
}


void ApplicationProperties::Open(const std::wstring& filename, const bool silent/* = false*/, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger/* = nullptr*/)
{
    auto json_reader = JsonSpecFile::CreateReader(filename, std::move(message_logger), [&]() { return ConvertPre80SpecFile(filename); });

    try
    {
        json_reader->CheckVersion();
        json_reader->CheckFileType(JK::properties);

        CreateFromJsonWorker(*json_reader);
    }

    catch( const CSProException& exception )
    {
        json_reader->GetMessageLogger().RethrowException(filename, exception);
    }

    // report any warnings
    json_reader->GetMessageLogger().DisplayWarnings(silent);
}


void ApplicationProperties::Save(const std::wstring& filename) const
{
    auto json_writer = JsonSpecFile::CreateWriter(filename, JK::properties);

    WriteJson(*json_writer, false, true);

    json_writer->EndObject();
}


ApplicationProperties ApplicationProperties::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    ApplicationProperties application_properties;
    application_properties.CreateFromJsonWorker(json_node);
    return application_properties;
}


void ApplicationProperties::CreateFromJsonWorker(const JsonNode<wchar_t>& json_node)
{
    UseHtmlDialogs = json_node.GetOrDefault(JK::htmlDialogs, UseHtmlDialogs);

    if( json_node.Contains(JK::mapping) )
        m_mappingProperties = json_node.Get<MappingProperties>(JK::mapping);

    if( json_node.Contains(JK::paradata) )
        m_paradataProperties = json_node.Get<ParadataProperties>(JK::paradata);

    if( json_node.Contains(JK::json) )
        m_jsonProperties = json_node.Get<JsonProperties>(JK::json);

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

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_3) )
        ar & m_useHtmlComponentsInsteadOfNativeVersions;
}
