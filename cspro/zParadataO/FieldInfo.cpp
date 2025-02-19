#include "stdafx.h"
#include "FieldInfo.h"

using namespace Paradata;


// --------------------------------------------------------------------------
// FieldOccurrenceInfo
// --------------------------------------------------------------------------

void FieldOccurrenceInfo::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::FieldOccurrenceInfo)
            .AddColumn("record_occurrence", Table::ColumnType::Integer)
            .AddColumn("item_occurrence", Table::ColumnType::Integer)
            .AddColumn("subitem_occurrence", Table::ColumnType::Integer)
            .AddIndex("field_occurrence_info_index", { 0 });
        ;
}


FieldOccurrenceInfo::FieldOccurrenceInfo(std::vector<size_t> one_based_occurrences)
    :   m_oneBasedOccurrences(std::move(one_based_occurrences))
{
    ASSERT(m_oneBasedOccurrences.size() == 3);
}


long FieldOccurrenceInfo::Save(Log& log) const
{
    Table& field_occurrence_info_table = log.GetTable(ParadataTable::FieldOccurrenceInfo);
    long field_occurrence_info_id = 0;
    field_occurrence_info_table.Insert(&field_occurrence_info_id,
        static_cast<int>(m_oneBasedOccurrences[0]),
        static_cast<int>(m_oneBasedOccurrences[1]),
        static_cast<int>(m_oneBasedOccurrences[2])
    );

    return field_occurrence_info_id;
}



// --------------------------------------------------------------------------
// FieldInfo
// --------------------------------------------------------------------------

void FieldInfo::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::FieldInfo)
            .AddColumn("field_name", Table::ColumnType::Long)
            .AddColumn("field_occurrence_info", Table::ColumnType::Long)
            .AddIndex("field_info_index", { 0 });
        ;
}


FieldInfo::FieldInfo(std::shared_ptr<NamedObject> field, std::vector<size_t> one_based_occurrences)
    :   m_field(std::move(field)),
        m_fieldOccurrenceInfo(std::move(one_based_occurrences))
{
}


long FieldInfo::Save(Log& log) const
{
    Table& field_info_table = log.GetTable(ParadataTable::FieldInfo);
    long field_info_id = 0;
    field_info_table.Insert(&field_info_id,
        log.AddNamedObject(m_field.get()),
        m_fieldOccurrenceInfo.Save(log)
    );

    return field_info_id;
}



// --------------------------------------------------------------------------
// FieldValueInfo
// --------------------------------------------------------------------------

void FieldValueInfo::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::FieldValueInfo)
            .AddColumn("field_name", Table::ColumnType::Long)
            .AddColumn("special_type", Table::ColumnType::Integer)
                    .AddCode(SpecialType::NotSpecial, "not_special")
                    .AddCode(SpecialType::Notappl, "notappl")
                    .AddCode(SpecialType::Missing, "missing")
                    .AddCode(SpecialType::Default, "default")
                    .AddCode(SpecialType::Refused, "refused")
            .AddColumn("value", Table::ColumnType::Text)
            .AddIndex("field_value_index", { 0, 2 });
        ;
}


FieldValueInfo::FieldValueInfo(std::shared_ptr<NamedObject> field, const SpecialType special_type, std::string value)
    :   m_field(std::move(field)),
        m_specialType(special_type),
        m_value(std::move(value))
{
}


long FieldValueInfo::Save(Log& log) const
{
    Table& field_value_info_table = log.GetTable(ParadataTable::FieldValueInfo);
    long field_value_info_id = 0;
    field_value_info_table.Insert(&field_value_info_id,
        log.AddNamedObject(m_field.get()),
        static_cast<int>(m_specialType),
        m_value.c_str()
    );

    return field_value_info_id;
}



// --------------------------------------------------------------------------
// FieldValidationInfo
// --------------------------------------------------------------------------

void FieldValidationInfo::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::FieldValidationInfo)
            .AddColumn("field_name", Table::ColumnType::Long)
            .AddColumn("value_set_name", Table::ColumnType::Long, true)
            .AddColumn("notappl_allowed", Table::ColumnType::Boolean)
            .AddColumn("notappl_confirmation", Table::ColumnType::Boolean)
            .AddColumn("outofrange_allowed", Table::ColumnType::Boolean)
            .AddColumn("outofrange_confirmation", Table::ColumnType::Boolean)
            .AddIndex("field_validation_info_index", { 0 });
        ;
}

FieldValidationInfo::FieldValidationInfo(std::shared_ptr<NamedObject> field, std::shared_ptr<NamedObject> value_set, const bool notappl_allowed,
                                         const bool notappl_confirmation, const bool out_of_range_allowed, const bool out_of_range_confirmation)
    :   m_field(std::move(field)),
        m_valueSet(std::move(value_set)),
        m_notapplAllowed(notappl_allowed),
        m_notapplConfirmation(notappl_confirmation),
        m_outOfRangeAllowed(out_of_range_allowed),
        m_outOfRangeConfirmation(out_of_range_confirmation)
{
}


long FieldValidationInfo::Save(Log& log) const
{
    Table& field_validation_info_table = log.GetTable(ParadataTable::FieldValidationInfo);
    long field_validation_info_id = 0;
    field_validation_info_table.Insert(&field_validation_info_id,
        log.AddNamedObject(m_field.get()),
        GetOptionalValueOrNull(log.AddNullableNamedObject(m_valueSet.get())),
        m_notapplAllowed,
        m_notapplConfirmation,
        m_outOfRangeAllowed,
        m_outOfRangeConfirmation
    );

    return field_validation_info_id;
}



// --------------------------------------------------------------------------
// FieldEntryInstance
// --------------------------------------------------------------------------

void FieldEntryInstance::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::FieldEntryInstance)
            .AddColumn("field_info", Table::ColumnType::Long)
        ;
}


FieldEntryInstance::FieldEntryInstance(std::shared_ptr<FieldInfo> field_info)
    :   m_fieldInfo(std::move(field_info))
{
}


long FieldEntryInstance::Save(Log& log)
{
    if( !m_id.has_value() )
    {
        Table& field_entry_instance_table = log.GetTable(ParadataTable::FieldEntryInstance);
        long id = 0;
        field_entry_instance_table.Insert(&id,
            m_fieldInfo->Save(log)
        );

        m_id = id;
    }

    return *m_id;
}
