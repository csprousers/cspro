#include "StdAfx.h"
#include "DictValuePair.h"


DictValuePair::DictValuePair(const CString& from/* = CString()*/, const CString& to/* = CString()*/)
    :   m_from(from),
        m_to(to)
{
}


DictValuePair DictValuePair::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    if( json_node.Contains(JK::value) )
    {
        return DictValuePair(json_node.Get<CString>(JK::value));
    }

    else
    {
        const JsonNodeArray<wchar_t> range_node = json_node.GetArray(JK::range);

        if( range_node.size() != 2 )
            throw JsonParseException(_T("Value ranges must contain exactly 2 entries, not %d"), static_cast<int>(range_node.size()));

        return DictValuePair(range_node[0].Get<CString>(), range_node[1].Get<CString>());
    }
}


void DictValuePair::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    if( m_to.IsEmpty() )
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
