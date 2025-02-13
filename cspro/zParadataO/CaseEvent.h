#pragma once
#include "Event.h"

namespace Paradata
{
    class KeyingInstance;

    class ZPARADATAO_API CaseEvent : public Event
    {
        DECLARE_PARADATA_EVENT(CaseEvent)

    private:
        bool PreSave(Log& log) const override;

        bool m_start;
        std::shared_ptr<KeyingInstance> m_keyingInstance;

        std::shared_ptr<NamedObject> m_dictionary;
        CString m_caseUuid;

        CaseEvent(bool start, std::shared_ptr<NamedObject> dictionary = nullptr);

    public:
        static std::shared_ptr<CaseEvent> CreateStartEvent(std::shared_ptr<NamedObject> dictionary, const CString& case_uuid);
        static std::shared_ptr<CaseEvent> CreateStopEvent();
    };
}
