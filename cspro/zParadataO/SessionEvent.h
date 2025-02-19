#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class SessionEvent; }


class ZPARADATAO_API Paradata::SessionEvent : public Event
{
    DECLARE_PARADATA_EVENT(SessionEvent)

private:
    SessionEvent(bool start);

public:
    static std::unique_ptr<SessionEvent> CreateStartEvent();
    static std::unique_ptr<SessionEvent> CreateStartEvent(int mode, std::string operator_id);
    static std::unique_ptr<SessionEvent> CreateStopEvent();

private:
    bool PreSave(Log& log) const override;

private:
    bool m_start;

    std::optional<int> m_mode;
    std::optional<std::string> m_operatorId;
};
