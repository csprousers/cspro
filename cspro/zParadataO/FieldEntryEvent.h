#pragma once

#include <zParadataO/Event.h>
#include <zParadataO/FieldInfo.h>
#include <zParadataO/FieldMovementEvent.h>

namespace Paradata { class FieldEntryEvent; class Table; }


class ZPARADATAO_API Paradata::FieldEntryEvent : public Event
{
    DECLARE_PARADATA_EVENT(FieldEntryEvent)

public:
    FieldEntryEvent(std::shared_ptr<FieldMovementInstance> arrival_field_movement_instance,
                    std::shared_ptr<FieldValidationInfo> field_validation_info,
                    int requested_capture_type, int actual_capture_type);

    void SetPostEntryValues();

private:
    static Table& AddCaptureType(const char* column_name, Table& table);

private:
    std::shared_ptr<FieldMovementInstance> m_arrivalFieldMovementInstance;
    std::shared_ptr<FieldValidationInfo> m_fieldValidationInfo;
    int m_requestedCaptureType;
    int m_actualCaptureType;
    double m_displayDuration;
};
