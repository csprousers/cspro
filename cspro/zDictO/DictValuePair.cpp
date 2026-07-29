#include "StdAfx.h"
#include "DictValuePair.h"


DictValuePair::DictValuePair(std::string from/* = std::string()*/, std::string to/* = std::string()*/)
    :   m_from(std::move(from)),
        m_to(std::move(to))
{
}


bool DictValuePair::operator==(const DictValuePair& rhs) const noexcept
{
    return ( m_from == rhs.m_from &&
             m_to == rhs.m_to );
}


DictValuePair DictValuePair::CreateFromJson(const JsonNode& json_node)
{
    if( json_node.Contains(JK::value) )
    {
        return DictValuePair(json_node.Get<std::string>(JK::value));
    }

    else
    {
        const JsonNodeArray range_node = json_node.GetArray(JK::range);

        if( range_node.size() != 2 )
            throw JsonParseException("Value ranges must contain exactly 2 entries, not %zu", range_node.size());

        return DictValuePair(range_node[0].Get<std::string>(), range_node[1].Get<std::string>());
    }
}


void DictValuePair::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    if( m_to.empty() )
    {
        json_writer.Write(JK::value, SO::TrimRight(m_from));
    }

    else
    {
        json_writer.BeginArray(JK::range)
                   .Write(SO::TrimRight(m_from))
                   .Write(SO::TrimRight(m_to))
                   .EndArray();
    }

    json_writer.EndObject();
}


void DictValuePair::serialize(Serializer& ar)
{
    ar & m_from;
    ar & m_to;
}
