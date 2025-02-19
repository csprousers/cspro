#pragma once

#include <zAppO/zAppO.h>


class ZAPPO_API Language
{
public:
    static constexpr const char* DefaultName  = "EN";
    static constexpr const char* DefaultLabel = "English";

    Language(std::string name = DefaultName, std::string label = DefaultLabel);

    bool operator==(const Language& rhs) const;
    bool operator!=(const Language& rhs) const { return !operator==(rhs); }

    const std::string& GetName() const { return m_name; }
    void SetName(std::string name)     { m_name = std::move(name); }

    const std::string& GetLabel() const { return m_label; }
    void SetLabel(std::string label)    { m_label = std::move(label); }

    // serialization
    static Language CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    std::string m_name;
    std::string m_label;
};
