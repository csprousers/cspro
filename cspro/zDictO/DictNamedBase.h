#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DictBase.h>


// a common parent class for dictionary elements that have names (and labels)

class CLASS_DECL_ZDICTO DictNamedBase : public DictBase
{
public:
    virtual ~DictNamedBase() { }

    virtual std::unique_ptr<DictNamedBase> Clone() const = 0;

    const CString& GetName() const    { return m_name; }
    void SetName(const CString& name) { m_name = name; }

    const std::set<CString>& GetAliases() const { return m_aliases; }
    void SetAliases(std::set<CString> aliases)  { m_aliases = std::move(aliases); }

protected:
    DictNamedBase& operator=(const DictNamedBase& rhs);

    // serialization
    void ParseJsonInput(const JsonNode<wchar_t>& json_node, bool also_parse_dict_base = true);
    void WriteJson(JsonWriter& json_writer, bool also_write_dict_base = true) const;

    void serialize(Serializer& ar);

private:
    CString m_name;
    std::set<CString> m_aliases;
};
