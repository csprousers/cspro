#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class PropertyEvent; }


class ZPARADATAO_API Paradata::PropertyEvent : public Event
{
    DECLARE_PARADATA_EVENT(PropertyEvent)

public:
    PropertyEvent(std::string property, std::string value, bool user_modified, std::shared_ptr<NamedObject> dict_item = nullptr);

private:
    std::string m_property;
    std::string m_value;
    bool m_userModified;
    std::shared_ptr<NamedObject> m_dictItem;
};
