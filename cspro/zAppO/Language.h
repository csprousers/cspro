#pragma once

#include <zAppO/zAppO.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


class ZAPPO_API Language
{
public:
    constexpr static const TCHAR* DefaultName  = _T("EN");
    constexpr static const TCHAR* DefaultLabel = _T("English");

    Language(std::wstring name = DefaultName, std::wstring label = DefaultLabel);

    bool operator==(const Language& rhs) const;
    bool operator!=(const Language& rhs) const { return !operator==(rhs); }

    const std::wstring& GetName() const { return m_name; }
    void SetName(std::wstring name)     { m_name = std::move(name); }

    const std::wstring& GetLabel() const { return m_label; }
    void SetLabel(std::wstring label)    { m_label = std::move(label); }

    // serialization
    static Language CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);

private:
    std::wstring m_name;
    std::wstring m_label;
};
