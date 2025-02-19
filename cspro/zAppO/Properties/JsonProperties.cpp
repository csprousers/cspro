#include "stdafx.h"
#include "JsonProperties.h"
#include <zDataO/ConnectionStringProperties.h>


bool JsonProperties::operator==(const JsonProperties& rhs) const
{
    return ( m_jsonFormat == rhs.m_jsonFormat &&
             m_arrayFormat == rhs.m_arrayFormat &&
             m_hashMapFormat == rhs.m_hashMapFormat &&
             m_binaryDataFormat == rhs.m_binaryDataFormat );
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_JSON_KEY(ArrayFormat)
CREATE_JSON_KEY(HashMapFormat)

CREATE_ENUM_JSON_SERIALIZER(JsonProperties::JsonFormat,
    { JsonProperties::JsonFormat::Compact, CSValue::compact },
    { JsonProperties::JsonFormat::Pretty,  CSValue::pretty })

CREATE_ENUM_JSON_SERIALIZER(JsonProperties::ArrayFormat,
    { JsonProperties::ArrayFormat::Full,   "full" },
    { JsonProperties::ArrayFormat::Sparse, "sparse" })

CREATE_ENUM_JSON_SERIALIZER(JsonProperties::HashMapFormat,
    { JsonProperties::HashMapFormat::Array,  "array" },
    { JsonProperties::HashMapFormat::Object, "object" })

CREATE_ENUM_JSON_SERIALIZER(JsonProperties::BinaryDataFormat,
    { JsonProperties::BinaryDataFormat::DataUrl,      "dataUrl" },
    { JsonProperties::BinaryDataFormat::LocalhostUrl, "localhostUrl" })


JsonProperties JsonProperties::CreateFromJson(const JsonNode& json_node)
{
    JsonProperties json_properties;
    json_properties.UpdateFromJson(json_node);
    return json_properties;
}


void JsonProperties::UpdateFromJson(const JsonNode& json_node)
{
    m_jsonFormat = json_node.GetOrDefault(JK::jsonFormat, m_jsonFormat);
    m_arrayFormat = json_node.GetOrDefault(JK::ArrayFormat, m_arrayFormat);
    m_hashMapFormat = json_node.GetOrDefault(JK::HashMapFormat, m_hashMapFormat);
    m_binaryDataFormat = json_node.GetOrDefault(JK::binaryDataFormat, m_binaryDataFormat);
}


void JsonProperties::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::jsonFormat, m_jsonFormat)
               .Write(JK::ArrayFormat, m_arrayFormat)
               .Write(JK::HashMapFormat, m_hashMapFormat)
               .Write(JK::binaryDataFormat, m_binaryDataFormat)
               .EndObject();
}


void JsonProperties::serialize(Serializer& ar)
{
    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_4) )
        ar.SerializeEnum(m_jsonFormat);

    ar.SerializeEnum(m_arrayFormat)
      .SerializeEnum(m_hashMapFormat)
      .SerializeEnum(m_binaryDataFormat);
}
