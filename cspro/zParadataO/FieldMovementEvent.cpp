#include "stdafx.h"
#include "FieldMovementEvent.h"

using namespace Paradata;


// --------------------------------------------------------------------------
// FieldMovementInstance
// --------------------------------------------------------------------------

void FieldMovementInstance::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::FieldMovementTypeInfo)
            .AddColumn("request_type", Table::ColumnType::Integer)
                    .AddCode(FieldMovementTypeInfo::RequestType::Advance, "advance")
                    .AddCode(FieldMovementTypeInfo::RequestType::Skip, "skip")
                    .AddCode(FieldMovementTypeInfo::RequestType::Reenter, "reenter")
                    .AddCode(FieldMovementTypeInfo::RequestType::AdvanceToNext, "advance_next")
                    .AddCode(FieldMovementTypeInfo::RequestType::SkipToNext, "skip_next")
                    .AddCode(FieldMovementTypeInfo::RequestType::EndOccurrence, "end_occurrence")
                    .AddCode(FieldMovementTypeInfo::RequestType::EndGroup, "endgroup")
                    .AddCode(FieldMovementTypeInfo::RequestType::EndLevel, "endlevel")
                    .AddCode(FieldMovementTypeInfo::RequestType::EndFlow, "end_flow")
                    .AddCode(FieldMovementTypeInfo::RequestType::NextField, "next_field")
                    .AddCode(FieldMovementTypeInfo::RequestType::PreviousField, "previous_field")
            .AddColumn("forward_movement", Table::ColumnType::Boolean)
            .AddIndex("field_movement_type_info_index", { 0 });
        ;

    log.CreateTable(ParadataTable::FieldMovementInstance)
            .AddColumn("from_field_entry_instance", Table::ColumnType::Long, true)
            .AddColumn("initial_field_movement_type_info", Table::ColumnType::Long, true)
            .AddColumn("final_field_movement_type_info", Table::ColumnType::Long, true)
            .AddColumn("to_field_entry_instance", Table::ColumnType::Long, true)
        ;
}


FieldMovementInstance::FieldMovementInstance(std::shared_ptr<FieldEntryInstance> from_field_entry_instance, std::shared_ptr<FieldMovementTypeInfo> initial_field_movement_type,
                                             std::shared_ptr<FieldMovementTypeInfo> final_field_movement_type, std::shared_ptr<FieldEntryInstance> to_field_entry_instance)
    :   m_fromFieldEntryInstance(std::move(from_field_entry_instance)),
        m_initialFieldMovementType(std::move(initial_field_movement_type)),
        m_finalFieldMovementType(std::move(final_field_movement_type)),
        m_toFieldEntryInstance(std::move(to_field_entry_instance))
{
}


long FieldMovementInstance::Save(Log& log)
{
    if( !m_id.has_value() )
    {
        // save the field entry instances
        auto save_field_entry_instance = [&](const std::shared_ptr<FieldEntryInstance>& field_entry_instance)
        {
            return ( field_entry_instance != nullptr ) ? std::make_optional(field_entry_instance->Save(log)) :
                                                         std::nullopt;
        };

        // save the movement info
        Table& field_movement_type_info_table = log.GetTable(ParadataTable::FieldMovementTypeInfo);

        auto save_field_movement_type_info = [&](const std::shared_ptr<FieldMovementTypeInfo>& field_movement_type)
        {
            std::optional<long> field_movement_type_info_id;

            if( field_movement_type != nullptr )
            {
                field_movement_type_info_id.emplace(0);
                field_movement_type_info_table.Insert(&(*field_movement_type_info_id),
                    static_cast<int>(field_movement_type->request_type),
                    field_movement_type->forward_movement
                );
            }

            return field_movement_type_info_id;
        };

        // save the field movement instance
        Table& field_movement_instance_table = log.GetTable(ParadataTable::FieldMovementInstance);
        long id = 0;
        field_movement_instance_table.Insert(&id,
            GetOptionalValueOrNull(save_field_entry_instance(m_fromFieldEntryInstance)),
            GetOptionalValueOrNull(save_field_movement_type_info(m_initialFieldMovementType)),
            GetOptionalValueOrNull(save_field_movement_type_info(m_finalFieldMovementType)),
            GetOptionalValueOrNull(save_field_entry_instance(m_toFieldEntryInstance))
        );

        m_id = id;
    }

    return *m_id;
}



// --------------------------------------------------------------------------
// FieldMovementEvent
// --------------------------------------------------------------------------

void FieldMovementEvent::SetupTables(Log& log)
{
    FieldMovementInstance::SetupTables(log);

    log.CreateTable(ParadataTable::FieldMovementEvent)
            .AddColumn("field_movement_instance", Table::ColumnType::Long)
        ;
}


FieldMovementEvent::FieldMovementEvent(std::shared_ptr<FieldMovementInstance> field_movement_instance)
    :   m_fieldMovementInstance(std::move(field_movement_instance))
{
}


void FieldMovementEvent::Save(Log& log, long base_event_id) const
{
    Table& field_movement_event_table = log.GetTable(ParadataTable::FieldMovementEvent);
    field_movement_event_table.Insert(&base_event_id,
        m_fieldMovementInstance->Save(log)
    );
}
