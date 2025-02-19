#include "StdAfx.h"
#include "DictNamedBase.h"


DictNamedBase& DictNamedBase::operator=(const DictNamedBase& rhs)
{
    DictBase::operator=(rhs);

    m_name = rhs.m_name;
    m_aliases = rhs.m_aliases;

    return *this;
}


void DictNamedBase::ParseJsonInput(const JsonNode& json_node, const bool also_parse_dict_base/* = true*/)
{
    m_name = SO::ToUpper(json_node.Get<std::string_view>(JK::name));

    if( json_node.Contains(JK::aliases) )
    {
        for( const JsonNode& alias_node : json_node.GetArray(JK::aliases) )
            m_aliases.insert(SO::ToUpper(alias_node.Get<std::string>()));
    }

    if( also_parse_dict_base )
        DictBase::ParseJsonInput(json_node);
}


void DictNamedBase::WriteJson(JsonWriter& json_writer, const bool also_write_dict_base/* = true*/) const
{
    json_writer.Write(JK::name, m_name);

    if( !m_aliases.empty() )
        json_writer.Write(JK::aliases, m_aliases);

    if( also_write_dict_base )
        DictBase::WriteJson(json_writer);
}


void DictNamedBase::serialize(Serializer& ar)
{
    DictBase::serialize(ar);

    ar & m_name
       & m_aliases;
}
