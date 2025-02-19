#include "stdafx.h"
#include "DictionaryDescription.h"


const char* ToString(const DictionaryType dictionary_type)
{
    switch( dictionary_type )
    {
        case DictionaryType::Input:     return "Input";
        case DictionaryType::External:  return "External";
        case DictionaryType::Output:    return "Output";
        case DictionaryType::Working:   return "Working";
        default:                        return "Unknown";
    }
}


DictionaryDescription::DictionaryDescription(std::string dictionary_file_path, std::string parent_file_path, const DictionaryType dictionary_type,
                                             const std::optional<bool> include_in_simple_synchronization/* = std::nullopt*/,
                                             const bool include_value_set_images_in_compiled_application/* = false*/)
    :   m_dictionaryFilePath(std::move(dictionary_file_path)),
        m_dictionaryType(dictionary_type),
        m_parentFilePath(std::move(parent_file_path)),
        m_includeInSimpleSynchronization(include_in_simple_synchronization.value_or(dictionary_type == DictionaryType::Input)),
        m_includeValueSetImagesInCompiledApplication(include_value_set_images_in_compiled_application),
        m_dictionary(nullptr)
{
}


DictionaryDescription::DictionaryDescription(std::string dictionary_file_path/* = std::string()*/, const DictionaryType dictionary_type/* = DictionaryType::Unknown*/)
    :   DictionaryDescription(std::move(dictionary_file_path), std::string(), dictionary_type)
{
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_JSON_KEY(includeValueSetImagesInCompiledApplication)
CREATE_JSON_KEY(simpleSynchronization)

CREATE_ENUM_JSON_SERIALIZER(DictionaryType,
    { DictionaryType::Input,    "input" },
    { DictionaryType::External, "external" },
    { DictionaryType::Output,   "output" },
    { DictionaryType::Working,  "working" })


DictionaryDescription DictionaryDescription::CreateFromJson(const JsonNode& json_node)
{
    return DictionaryDescription(json_node.GetAbsolutePath(json_node.Contains(JK::filename) ? JK::filename : JK::path),
                                 json_node.Contains(JK::parent) ? json_node.GetAbsolutePath(JK::parent) : std::string(),
                                 json_node.Get<DictionaryType>(JK::type),
                                 json_node.GetOptional<bool>(JK::simpleSynchronization),
                                 json_node.GetOrDefault(JK::includeValueSetImagesInCompiledApplication, false));
}


void DictionaryDescription::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::type, m_dictionaryType)
               .WriteRelativePath(JK::path, m_dictionaryFilePath);

    if( json_writer.Verbose() || !m_parentFilePath.empty() )
        json_writer.WriteRelativePath(JK::parent, m_parentFilePath);

    json_writer.Write(JK::simpleSynchronization, m_includeInSimpleSynchronization)
               .Write(JK::includeValueSetImagesInCompiledApplication, m_includeValueSetImagesInCompiledApplication);

    json_writer.EndObject();
}


void DictionaryDescription::serialize(Serializer& ar)
{
    ar.SerializePath(m_dictionaryFilePath);
    ar.SerializeEnum(m_dictionaryType);
    ar.SerializePath(m_parentFilePath);

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        ar & m_includeInSimpleSynchronization
           & m_includeValueSetImagesInCompiledApplication;
    }

    else
    {
        m_includeInSimpleSynchronization = ( m_dictionaryType == DictionaryType::Input );
    }
}
