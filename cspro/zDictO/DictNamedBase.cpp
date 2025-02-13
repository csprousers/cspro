#include "StdAfx.h"
#include "DictNamedBase.h"


DictNamedBase& DictNamedBase::operator=(const DictNamedBase& rhs)
{
    DictBase::operator=(rhs);

    m_name = rhs.m_name;
    m_aliases = rhs.m_aliases;

    return *this;
}


void DictNamedBase::ParseJsonInput(const JsonNode<wchar_t>& json_node, bool also_parse_dict_base/* = true*/)
{
    m_name = json_node.Get<CString>(JK::name);
    m_name.MakeUpper();

    if( json_node.Contains(JK::aliases) )
    {
        for( const auto& alias_node : json_node.GetArray(JK::aliases) )
            m_aliases.insert(alias_node.Get<CString>().MakeUpper());
    }

    if( also_parse_dict_base )
        DictBase::ParseJsonInput(json_node);
}


void DictNamedBase::WriteJson(JsonWriter& json_writer, bool also_write_dict_base/* = true*/) const
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

    ar & m_name;

    if( ar.MeetsVersionIteration(Serializer::Iteration_7_7_000_1) )
        ar & m_aliases;
}
