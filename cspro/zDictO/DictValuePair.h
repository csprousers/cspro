#pragma once

#include <zDictO/zDictO.h>


class CLASS_DECL_ZDICTO DictValuePair
{
public:
    DictValuePair(std::string from = std::string(), std::string to = std::string());

    bool operator==(const DictValuePair& rhs) const noexcept;
    bool operator!=(const DictValuePair& rhs) const noexcept { return !( *this == rhs ); }

    const std::string& GetFrom() const { return m_from; }
    void SetFrom(std::string from)     { m_from = std::move(from); }

    const std::string& GetTo() const { return m_to; }
    void SetTo(std::string to)       { m_to = std::move(to); }


    // serialization
    static DictValuePair CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    std::string m_from;
    std::string m_to;
};
