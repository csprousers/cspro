#pragma once

#include <zDictO/zDictO.h>

class Serializer;


class CLASS_DECL_ZDICTO DictValuePair
{
public:
    DictValuePair(const CString& from = CString(), const CString& to = CString());

    const CString& GetFrom() const    { return m_from; }
    void SetFrom(const CString& from) { m_from = from; }

    const CString& GetTo() const  { return m_to; }
    void SetTo(const CString& to) { m_to = to; }


    // serialization
    static DictValuePair CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    CString m_from;
    CString m_to;
};
