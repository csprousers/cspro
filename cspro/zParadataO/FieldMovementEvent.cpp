#include "stdafx.h"
#include "FieldMovementEvent.h"

namespace Paradata
{
    void FieldMovementInstance::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::FieldMovementTypeInfo)
                .AddColumn(_T("request_type"), Table::ColumnType::Integer)
                        .AddCode((int)FieldMovementTypeInfo::RequestType::Advance, _T("advance"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::Skip, _T("skip"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::Reenter, _T("reenter"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::AdvanceToNext, _T("advance_next"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::SkipToNext, _T("skip_next"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::EndOccurrence, _T("end_occurrence"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::EndGroup, _T("endgroup"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::EndLevel, _T("endlevel"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::EndFlow, _T("end_flow"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::NextField, _T("next_field"))
                        .AddCode((int)FieldMovementTypeInfo::RequestType::PreviousField, _T("previous_field"))
                .AddColumn(_T("forward_movement"), Table::ColumnType::Boolean)
                .AddIndex(_T("field_movement_type_info_index"), { 0 });
            ;

        log.CreateTable(ParadataTable::FieldMovementInstance)
                .AddColumn(_T("from_field_entry_instance"), Table::ColumnType::Long, true)
                .AddColumn(_T("initial_field_movement_type_info"), Table::ColumnType::Long, true)
                .AddColumn(_T("final_field_movement_type_info"), Table::ColumnType::Long, true)
                .AddColumn(_T("to_field_entry_instance"), Table::ColumnType::Long, true)
            ;
    }

    FieldMovementInstance::FieldMovementInstance(std::shared_ptr<FieldEntryInstance> from_field_entry_instance,
        std::shared_ptr<FieldMovementTypeInfo> initial_field_movement_type, std::shared_ptr<FieldMovementTypeInfo> final_field_movement_type,
        std::shared_ptr<FieldEntryInstance> to_field_entry_instance)
        :   m_fromFieldEntryInstance(from_field_entry_instance),
            m_initialFieldMovementType(initial_field_movement_type),
            m_finalFieldMovementType(final_field_movement_type),
            m_toFieldEntryInstance(to_field_entry_instance)
    {
    }

    long FieldMovementInstance::Save(Log& log)
    {
        if( !m_id.has_value() )
        {
            // save the field entry instances
            auto save_field_entry_instance = [&](const std::shared_ptr<FieldEntryInstance>& field_entry_instance) -> std::optional<long>
            {
                return ( field_entry_instance != nullptr ) ? (std::optional<long>)field_entry_instance->Save(log) : std::nullopt;
            };

            // save the movement info
            Table& field_movement_type_info_table = log.GetTable(ParadataTable::FieldMovementTypeInfo);

            auto save_field_movement_type_info = [&](const std::shared_ptr<FieldMovementTypeInfo>& field_movement_type) -> std::optional<long>
            {
                if( field_movement_type == nullptr )
                    return std::nullopt;

                long field_movement_type_info_id = 0;
                field_movement_type_info_table.Insert(&field_movement_type_info_id,
                    (int)field_movement_type->request_type,
                    field_movement_type->forward_movement
                );

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


    // --------------------------------------------------------------

    void FieldMovementEvent::SetupTables(Log& log)
    {
        FieldMovementInstance::SetupTables(log);

        log.CreateTable(ParadataTable::FieldMovementEvent)
                .AddColumn(_T("field_movement_instance"), Table::ColumnType::Long)
            ;
    }

    FieldMovementEvent::FieldMovementEvent(std::shared_ptr<FieldMovementInstance> field_movement_instance)
        :   m_fieldMovementInstance(field_movement_instance)
    {
    }

    void FieldMovementEvent::Save(Log& log, long base_event_id) const
    {
        Table& field_movement_event_table = log.GetTable(ParadataTable::FieldMovementEvent);
        field_movement_event_table.Insert(&base_event_id,
            m_fieldMovementInstance->Save(log)
        );
    }
}
