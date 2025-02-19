#pragma once

#include <zAppO/zAppO.h>
#include <zAppO/Properties/JavaScriptProperties.h>
#include <zAppO/Properties/JsonProperties.h>
#include <zAppO/Properties/MappingProperties.h>
#include <zAppO/Properties/ParadataProperties.h>

namespace JsonSpecFile { class ReaderMessageLogger; }


class ZAPPO_API ApplicationProperties
{
    friend class Application;

public:
    ApplicationProperties();

    bool operator==(const ApplicationProperties& rhs) const;
    bool operator!=(const ApplicationProperties& rhs) const { return !( *this == rhs ); }

    const ParadataProperties& GetParadataProperties() const { return m_paradataProperties; }
    ParadataProperties& GetParadataProperties()             { return m_paradataProperties; }

    const MappingProperties& GetMappingProperties() const { return m_mappingProperties; }
    MappingProperties& GetMappingProperties()             { return m_mappingProperties; }

    const JsonProperties& GetJsonProperties() const { return m_jsonProperties; }
    JsonProperties& GetJsonProperties()             { return m_jsonProperties; }

    const JavaScriptProperties& GetJavaScriptProperties() const { return m_javascriptProperties; }
    JavaScriptProperties& GetJavaScriptProperties()             { return m_javascriptProperties; }

    bool GetUseHtmlComponentsInsteadOfNativeVersions() const    { return m_useHtmlComponentsInsteadOfNativeVersions; }
    void SetUseHtmlComponentsInsteadOfNativeVersions(bool flag) { m_useHtmlComponentsInsteadOfNativeVersions = flag; }


    // serialization
    // --------------------------------------------------------------------------

    // all serialization methods (with the exception of WriteJson and serialize) can throw exceptions
    void Open(InterfaceString file_path, bool silent = false, std::shared_ptr<JsonSpecFile::ReaderMessageLogger> message_logger = nullptr);
    void Save(InterfaceString file_path) const;

    static ApplicationProperties CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer, bool write_to_new_json_object = true, bool write_sections_if_all_default_values = true) const;

    void serialize(Serializer& ar);

private:
    void CreateFromJsonWorker(const JsonNode& json_node);

    static std::string ConvertPre80SpecFile(InterfaceString file_path);


private:
    ParadataProperties m_paradataProperties;
    MappingProperties m_mappingProperties;
    JsonProperties m_jsonProperties;
    JavaScriptProperties m_javascriptProperties;
    bool m_useHtmlComponentsInsteadOfNativeVersions;

    // temporary...
public:
    bool UseHtmlDialogs = true;
};
