#include "stdafx.h"
#include "FieldValidationEvent.h"

namespace Paradata
{
    void FieldValidationEvent::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::FieldValidationEvent)
                .AddColumn(_T("field_info"), Table::ColumnType::Long)
                .AddColumn(_T("field_validation_info"), Table::ColumnType::Long)
                .AddColumn(_T("field_value_info"), Table::ColumnType::Long, true)
                .AddColumn(_T("field_entry_instance"), Table::ColumnType::Long, true)
                .AddColumn(_T("invalueset"), Table::ColumnType::Boolean)
                .AddColumn(_T("operator_confirmed"), Table::ColumnType::Boolean, true)
                .AddColumn(_T("onrefused_result"), Table::ColumnType::Double, true)
                .AddColumn(_T("validated"), Table::ColumnType::Boolean)
            ;
    }

    FieldValidationEvent::FieldValidationEvent(std::shared_ptr<FieldInfo> field_info, std::shared_ptr<FieldValidationInfo> field_validation_info,
        std::shared_ptr<FieldValueInfo> field_value_info, std::shared_ptr<FieldEntryInstance> field_entry_instance)
        :   m_fieldInfo(field_info),
            m_fieldValidationInfo(field_validation_info),
            m_fieldValueInfo(field_value_info),
            m_fieldEntryInstance(field_entry_instance),
            m_inValueSet(false),
            m_validated(false)
    {
    }

    void FieldValidationEvent::SetInValueSet(bool in_value_set)
    {
        m_inValueSet = in_value_set;
        m_validated |= in_value_set;
    }

    void FieldValidationEvent::SetOnRefusedResult(double on_refused_result)
    {
        m_onRefusedResult = on_refused_result;

        if( on_refused_result == 0 )
            m_validated = false;
    }

    void FieldValidationEvent::SetOperatorConfirmed(bool operator_confirmed)
    {
        m_operatorConfirmed = operator_confirmed;
        m_validated |= operator_confirmed;
    }

    void FieldValidationEvent::Save(Log& log, long base_event_id) const
    {
        std::optional<long> field_value_info_id = ( m_fieldValueInfo != nullptr ) ?
            (std::optional<long>)m_fieldValueInfo->Save(log) : std::nullopt;

        std::optional<long> field_entry_instance_id = ( m_fieldEntryInstance != nullptr ) ?
            (std::optional<long>)m_fieldEntryInstance->Save(log) : std::nullopt;

        Table& field_validation_event_table = log.GetTable(ParadataTable::FieldValidationEvent);
        field_validation_event_table.Insert(&base_event_id,
            m_fieldInfo->Save(log),
            m_fieldValidationInfo->Save(log),
            GetOptionalValueOrNull(field_value_info_id),
            GetOptionalValueOrNull(field_entry_instance_id),
            m_inValueSet,
            GetOptionalValueOrNull(m_operatorConfirmed),
            GetOptionalValueOrNull(m_onRefusedResult),
            m_validated
        );
    }
}
