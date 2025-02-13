#include "stdafx.h"
#include "Language.h"


Language::Language(std::wstring name/* = DefaultName*/, std::wstring label/* = DefaultLabel*/)
    :   m_name(std::move(name)),
        m_label(std::move(label))
{
    ASSERT(SO::IsUpper(m_name) && CIMSAString::IsName(m_name));
}


bool Language::operator==(const Language& rhs) const
{
    return ( m_name == rhs.m_name &&
             m_label == rhs.m_label );
}


Language Language::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    return Language(json_node.Get<std::wstring>(JK::name),
                    json_node.Get<std::wstring>(JK::label));
}

void Language::WriteJson(JsonWriter& json_writer) const
{
    json_writer
        .BeginObject()
        .Write(JK::name, m_name)
        .Write(JK::label, m_label)
        .EndObject();
}


void Language::serialize(Serializer& ar)
{
    ar & m_name
       & m_label;
}
