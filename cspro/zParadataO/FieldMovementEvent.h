#pragma once
#include "Event.h"
#include "FieldInfo.h"

namespace Paradata
{
    struct FieldMovementTypeInfo
    {
        enum class RequestType
        {
            Advance,
            Skip,
            Reenter,
            AdvanceToNext,
            SkipToNext,
            EndOccurrence,
            EndGroup,
            EndLevel,
            EndFlow,
            NextField,
            PreviousField
        };

        RequestType request_type;
        bool forward_movement;
    };

    class ZPARADATAO_API FieldMovementInstance
    {
        DECLARE_PARADATA_SHARED_PTR_INSTANCE()

        friend class FieldMovementEvent;

    private:
        std::shared_ptr<FieldEntryInstance> m_fromFieldEntryInstance;
        std::shared_ptr<FieldMovementTypeInfo> m_initialFieldMovementType;
        std::shared_ptr<FieldMovementTypeInfo> m_finalFieldMovementType;
        std::shared_ptr<FieldEntryInstance> m_toFieldEntryInstance;
        std::optional<long> m_id;

    public:
        FieldMovementInstance(std::shared_ptr<FieldEntryInstance> from_field_entry_instance, std::shared_ptr<FieldMovementTypeInfo> initial_field_movement_type,
            std::shared_ptr<FieldMovementTypeInfo> final_field_movement_type, std::shared_ptr<FieldEntryInstance> to_field_entry_instance);

        std::shared_ptr<FieldEntryInstance> GetToFieldEntryInstance() { return m_toFieldEntryInstance; }
    };


    class ZPARADATAO_API FieldMovementEvent : public Event
    {
        DECLARE_PARADATA_EVENT(FieldMovementEvent)

    private:
        std::shared_ptr<FieldMovementInstance> m_fieldMovementInstance;

    public:
        FieldMovementEvent(std::shared_ptr<FieldMovementInstance> field_movement_instance);
    };
}
