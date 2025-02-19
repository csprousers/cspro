#include "stdafx.h"
#include "FieldValidationEvent.h"

using namespace Paradata;


void FieldValidationEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::FieldValidationEvent)
            .AddColumn("field_info", Table::ColumnType::Long)
            .AddColumn("field_validation_info", Table::ColumnType::Long)
            .AddColumn("field_value_info", Table::ColumnType::Long, true)
            .AddColumn("field_entry_instance", Table::ColumnType::Long, true)
            .AddColumn("invalueset", Table::ColumnType::Boolean)
            .AddColumn("operator_confirmed", Table::ColumnType::Boolean, true)
            .AddColumn("onrefused_result", Table::ColumnType::Double, true)
            .AddColumn("validated", Table::ColumnType::Boolean)
        ;
}


FieldValidationEvent::FieldValidationEvent(std::shared_ptr<FieldInfo> field_info, std::shared_ptr<FieldValidationInfo> field_validation_info,
                                           std::shared_ptr<FieldValueInfo> field_value_info, std::shared_ptr<FieldEntryInstance> field_entry_instance)
    :   m_fieldInfo(std::move(field_info)),
        m_fieldValidationInfo(std::move(field_validation_info)),
        m_fieldValueInfo(std::move(field_value_info)),
        m_fieldEntryInstance(std::move(field_entry_instance)),
        m_inValueSet(false),
        m_validated(false)
{
}


void FieldValidationEvent::SetInValueSet(const bool in_value_set)
{
    m_inValueSet = in_value_set;
    m_validated |= in_value_set;
}


void FieldValidationEvent::SetOnRefusedResult(const double on_refused_result)
{
    m_onRefusedResult = on_refused_result;

    if( on_refused_result == 0 )
        m_validated = false;
}


void FieldValidationEvent::SetOperatorConfirmed(const bool operator_confirmed)
{
    m_operatorConfirmed = operator_confirmed;
    m_validated |= operator_confirmed;
}


void FieldValidationEvent::Save(Log& log, long base_event_id) const
{
    const std::optional<long> field_value_info_id = ( m_fieldValueInfo != nullptr ) ? std::make_optional(m_fieldValueInfo->Save(log)) :
                                                                                      std::nullopt;

    const std::optional<long> field_entry_instance_id = ( m_fieldEntryInstance != nullptr ) ? std::make_optional(m_fieldEntryInstance->Save(log)) :
                                                                                              std::nullopt;

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
