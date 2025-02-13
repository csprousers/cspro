#pragma once
#include "Event.h"

namespace Paradata
{
    class ZPARADATAO_API SessionEvent : public Event
    {
        DECLARE_PARADATA_EVENT(SessionEvent)

    private:
        bool PreSave(Log& log) const override;

        bool m_start;

        std::optional<int> m_mode;
        std::optional<CString> m_operatorId;

        SessionEvent(bool start);

    public:
        static std::shared_ptr<SessionEvent> CreateStartEvent();
        static std::shared_ptr<SessionEvent> CreateStartEvent(int mode, const CString& operator_id);
        static std::shared_ptr<SessionEvent> CreateStopEvent();
    };
}
