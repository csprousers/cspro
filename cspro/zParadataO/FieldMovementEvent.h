#pragma once

#include <zParadataO/Event.h>
#include <zParadataO/FieldInfo.h>

namespace Paradata { class FieldMovementEvent; class FieldMovementInstance; struct FieldMovementTypeInfo; }


// --------------------------------------------------------------------------
// FieldMovementTypeInfo
// --------------------------------------------------------------------------

struct Paradata::FieldMovementTypeInfo
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



// --------------------------------------------------------------------------
// FieldMovementInstance
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::FieldMovementInstance
{
    DECLARE_PARADATA_SHARED_PTR_INSTANCE()

    friend class FieldMovementEvent;

public:
    FieldMovementInstance(std::shared_ptr<FieldEntryInstance> from_field_entry_instance, std::shared_ptr<FieldMovementTypeInfo> initial_field_movement_type,
                          std::shared_ptr<FieldMovementTypeInfo> final_field_movement_type, std::shared_ptr<FieldEntryInstance> to_field_entry_instance);

    std::shared_ptr<FieldEntryInstance> GetToFieldEntryInstance() { return m_toFieldEntryInstance; }

private:
    std::shared_ptr<FieldEntryInstance> m_fromFieldEntryInstance;
    std::shared_ptr<FieldMovementTypeInfo> m_initialFieldMovementType;
    std::shared_ptr<FieldMovementTypeInfo> m_finalFieldMovementType;
    std::shared_ptr<FieldEntryInstance> m_toFieldEntryInstance;
    std::optional<long> m_id;
};



// --------------------------------------------------------------------------
// FieldMovementEvent
// --------------------------------------------------------------------------

class ZPARADATAO_API Paradata::FieldMovementEvent : public Event
{
    DECLARE_PARADATA_EVENT(FieldMovementEvent)

public:
    FieldMovementEvent(std::shared_ptr<FieldMovementInstance> field_movement_instance);

private:
    std::shared_ptr<FieldMovementInstance> m_fieldMovementInstance;
};
