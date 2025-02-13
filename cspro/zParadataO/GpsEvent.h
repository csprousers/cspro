#pragma once
#include "Event.h"

namespace Paradata
{
    struct GpsReadingInstance
    {
        std::optional<double> latitude;
        std::optional<double> longitude;
        std::optional<double> altitude;
        std::optional<double> satellites;
        std::optional<double> accuracy;
        std::optional<double> readtime;
    };

    class ZPARADATAO_API GpsEvent : public Event
    {
        DECLARE_PARADATA_EVENT(GpsEvent)

    public:
        enum class Action
        {
            Close,
            Open,
            Read,
            ReadLast,
            BackgroundReading,
            ReadInteractive,
            Select,
            BackgroundOpen, // will be logged as Open
            BackgroundClose // will be logged as Close
        };

    protected:
        Action m_action;
        std::optional<double> m_returnValue;
        mutable std::optional<long> m_gpsReadRequestInstanceId;
        std::optional<GpsReadingInstance> m_gpsReadingInstance;

    public:
        GpsEvent(Action action, const std::optional<GpsReadingInstance>& gps_reading_instance = std::nullopt);

        virtual void SetPostExecutionValues(double return_value, const std::optional<GpsReadingInstance>& gps_reading_instance = std::nullopt);
    };

    class ZPARADATAO_API GpsReadRequestEvent : public GpsEvent
    {
    private:
        int m_maxReadDuration;
        std::optional<int> m_desiredAccuracy;
        std::optional<CString> m_dialogText;
        double m_readDuration;

    public:
        GpsReadRequestEvent(Action action, int max_read_duration, const std::optional<int>& desired_accuracy,
            const std::optional<CString>& dialog_text);

        void SetPostExecutionValues(double return_value, const std::optional<GpsReadingInstance>& gps_reading_instance = std::nullopt) override;

        void Save(Log& log, long base_event_id) const override;
    };
}
