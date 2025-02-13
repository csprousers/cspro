#pragma once

#include <zParadataO/Event.h>


namespace Paradata
{
    class ZPARADATAO_API ExternalApplicationEvent : public Event
    {
        DECLARE_PARADATA_EVENT(ExternalApplicationEvent)

    public:
        enum class Source
        {
            ExecPff,
            ExecSystem,
            SystemAppExec
        };

        ExternalApplicationEvent(Source source, std::wstring action, bool stop);

        void SetPostExecutionValues(bool success, bool wait);

    private:
        Source m_source;
        std::wstring m_action;
        bool m_stop;
        bool m_success;
        std::optional<double> m_waitDuration;
    };
}
