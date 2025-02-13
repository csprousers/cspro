#include "stdafx.h"
#include "DictionaryDescription.h"


const TCHAR* ToString(DictionaryType dictionary_type)
{
    switch( dictionary_type )
    {
        case DictionaryType::Input:     return _T("Input");
        case DictionaryType::External:  return _T("External");
        case DictionaryType::Output:    return _T("Output");
        case DictionaryType::Working:   return _T("Working");
        default:                        return _T("Unknown");
    }
}


CREATE_ENUM_JSON_SERIALIZER(DictionaryType,
    { DictionaryType::Input,    _T("input") },
    { DictionaryType::External, _T("external") },
    { DictionaryType::Output,   _T("output") },
    { DictionaryType::Working,  _T("working") })


DictionaryDescription DictionaryDescription::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    DictionaryDescription dictionary_description(json_node.GetAbsolutePath(json_node.Contains(JK::filename) ? JK::filename : JK::path),
                                                 json_node.Get<DictionaryType>(JK::type));

    if( json_node.Contains(JK::parent) )
        dictionary_description.m_parentFilename = json_node.GetAbsolutePath(JK::parent);

    return dictionary_description;
}


void DictionaryDescription::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::type, m_dictionaryType)
               .WriteRelativePath(JK::path, m_dictionaryFilename);

    if( json_writer.Verbose() || !m_parentFilename.empty() )
        json_writer.WriteRelativePath(JK::parent, m_parentFilename);
    
    json_writer.EndObject();
}


void DictionaryDescription::serialize(Serializer& ar)
{
    ar.SerializeFilename(m_dictionaryFilename);
    ar.SerializeEnum(m_dictionaryType);
    ar.SerializeFilename(m_parentFilename);
}
