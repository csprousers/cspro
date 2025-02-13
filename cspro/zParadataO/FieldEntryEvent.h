#pragma once
#include "Event.h"
#include "FieldInfo.h"
#include "FieldMovementEvent.h"

namespace Paradata
{
    class ZPARADATAO_API FieldEntryEvent : public Event
    {
        DECLARE_PARADATA_EVENT(FieldEntryEvent)

    private:
        std::shared_ptr<FieldMovementInstance> m_arrivalFieldMovementInstance;
        std::shared_ptr<FieldValidationInfo> m_fieldValidationInfo;
        int m_requestedCaptureType;
        int m_actualCaptureType;
        double m_displayDuration;

    public:
        FieldEntryEvent(std::shared_ptr<FieldMovementInstance> arrival_field_movement_instance,
            std::shared_ptr<FieldValidationInfo> field_validation_info, int requested_capture_type, int actual_capture_type);

        void SetPostEntryValues();
    };
}
