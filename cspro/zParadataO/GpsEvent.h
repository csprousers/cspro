#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class GpsEvent; struct GpsReadingInstance; class GpsReadRequestEvent; }


// --------------------------------------------------------------------------
// GpsReadingInstance
// --------------------------------------------------------------------------

struct Paradata::GpsReadingInstance
{
    std::optional<double> latitude;
    std::optional<double> longitude;
    std::optional<double> altitude;
    std::optional<double> satellites;
    std::optional<double> accuracy;
    std::optional<double> readtime;
};



// --------------------------------------------------------------------------
// GpsEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::GpsEvent : public Event
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

public:
    GpsEvent(Action action, std::unique_ptr<GpsReadingInstance> gps_reading_instance = nullptr);

    virtual void SetPostExecutionValues(double return_value, std::unique_ptr<GpsReadingInstance> gps_reading_instance = nullptr);

protected:
    Action m_action;
    std::optional<double> m_returnValue;
    mutable std::optional<long> m_gpsReadRequestInstanceId;
    std::unique_ptr<GpsReadingInstance> m_gpsReadingInstance;
};



// --------------------------------------------------------------------------
// GpsReadRequestEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::GpsReadRequestEvent : public GpsEvent
{
public:
    GpsReadRequestEvent(Action action, int max_read_duration, std::optional<int> desired_accuracy, std::optional<std::string> dialog_text);

    void SetPostExecutionValues(double return_value, std::unique_ptr<GpsReadingInstance> gps_reading_instance = nullptr) override;

    void Save(Log& log, long base_event_id) const override;

private:
    int m_maxReadDuration;
    std::optional<int> m_desiredAccuracy;
    std::optional<std::string> m_dialogText;
    double m_readDuration;
};
