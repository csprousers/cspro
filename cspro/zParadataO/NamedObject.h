#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class NamedObject; class DataRepositoryOpenEvent; }


class Paradata::NamedObject
{
    friend class Log;

public:
    enum class Type
    {
        Other,
        Flow,
        Dictionary,
        Item,
        ValueSet,
        Language,
        Level,
        Record,
        Group,
        Block
    };

    NamedObject(Type type, std::string name, std::shared_ptr<NamedObject> parent = nullptr)
        :   m_type(type),
            m_name(std::move(name)),
            m_parent(std::move(parent))
    {
    }

    Type GetType() const               { return m_type; }
    const std::string& GetName() const { return m_name; }
    NamedObject* GetParent()           { return m_parent.get(); }

private:
    Type m_type;
    std::string m_name;
    std::shared_ptr<NamedObject> m_parent;
    std::optional<long> m_id;
};
