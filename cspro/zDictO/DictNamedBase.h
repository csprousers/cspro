#pragma once

#include <zDictO/zDictO.h>
#include <zDictO/DictBase.h>


// a common parent class for dictionary elements that have names (and labels)

class CLASS_DECL_ZDICTO DictNamedBase : public DictBase
{
public:
    virtual ~DictNamedBase() { }

    virtual std::unique_ptr<DictNamedBase> Clone() const = 0;

    const std::string& GetName() const { return m_name; }
    void SetName(std::string name)     { m_name = std::move(name); }

    const std::set<std::string>& GetAliases() const { return m_aliases; }
    void SetAliases(std::set<std::string> aliases)  { m_aliases = std::move(aliases); }

protected:
    DictNamedBase& operator=(const DictNamedBase& rhs);

    // serialization
    void ParseJsonInput(const JsonNode& json_node, bool also_parse_dict_base = true);
    void WriteJson(JsonWriter& json_writer, bool also_write_dict_base = true) const;

    void serialize(Serializer& ar);

private:
    std::string m_name;
    std::set<std::string> m_aliases;
};
