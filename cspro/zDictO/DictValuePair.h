#pragma once

#include <zDictO/zDictO.h>


class CLASS_DECL_ZDICTO DictValuePair
{
public:
    DictValuePair(const CString& from = CString(), const CString& to = CString());
    DictValuePair(std::string from, std::string to = std::string());

    bool operator==(const DictValuePair& rhs) const noexcept;
    bool operator!=(const DictValuePair& rhs) const noexcept { return !( *this == rhs ); }

    const CString& GetFrom() const    { return m_from; }
    void SetFrom(const CString& from) { m_from = from; }

    const CString& GetTo() const  { return m_to; }
    void SetTo(const CString& to) { m_to = to; }


    // serialization
    static DictValuePair CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    CString m_from;
    CString m_to;
};
