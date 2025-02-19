#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class ExternalApplicationEvent; }


class ZPARADATAO_API Paradata::ExternalApplicationEvent : public Event
{
    DECLARE_PARADATA_EVENT(ExternalApplicationEvent)

public:
    enum class Source
    {
        ExecPff,
        ExecSystem,
        SystemAppExec
    };

    ExternalApplicationEvent(Source source, std::string action, bool stop);

    void SetPostExecutionValues(bool success, bool wait);

private:
    Source m_source;
    std::string m_action;
    bool m_stop;
    bool m_success;
    std::optional<double> m_waitDuration;
};
