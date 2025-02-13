#pragma once
#include "Event.h"

namespace Paradata
{
    class ZPARADATAO_API PropertyEvent : public Event
    {
        DECLARE_PARADATA_EVENT(PropertyEvent)

    private:
        CString m_property;
        CString m_value;
        bool m_userModified;
        std::shared_ptr<NamedObject> m_dictionaryItem;

    public:
        PropertyEvent(const CString& property, const CString& value, bool user_modified, std::shared_ptr<NamedObject> dictionary_item = nullptr);
    };
}
