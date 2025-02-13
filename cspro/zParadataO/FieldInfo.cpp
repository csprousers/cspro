#include "stdafx.h"
#include "FieldInfo.h"

namespace Paradata
{
    void FieldOccurrenceInfo::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::FieldOccurrenceInfo)
                .AddColumn(_T("record_occurrence"), Table::ColumnType::Integer)
                .AddColumn(_T("item_occurrence"), Table::ColumnType::Integer)
                .AddColumn(_T("subitem_occurrence"), Table::ColumnType::Integer)
                .AddIndex(_T("field_occurrence_info_index"), { 0 });
            ;
    }

    FieldOccurrenceInfo::FieldOccurrenceInfo(const std::vector<size_t>& one_based_occurrences)
        :   m_oneBasedOccurrences(one_based_occurrences)
    {
        ASSERT(one_based_occurrences.size() == 3);
    }

    long FieldOccurrenceInfo::Save(Log& log) const
    {
        Table& field_occurrence_info_table = log.GetTable(ParadataTable::FieldOccurrenceInfo);
        long field_occurrence_info_id = 0;
        field_occurrence_info_table.Insert(&field_occurrence_info_id,
            (int)m_oneBasedOccurrences[0],
            (int)m_oneBasedOccurrences[1],
            (int)m_oneBasedOccurrences[2]
        );

        return field_occurrence_info_id;
    }

    // --------------------------------------------------------------


    void FieldInfo::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::FieldInfo)
                .AddColumn(_T("field_name"), Table::ColumnType::Long)
                .AddColumn(_T("field_occurrence_info"), Table::ColumnType::Long)
                .AddIndex(_T("field_info_index"), { 0 });
            ;
    }

    FieldInfo::FieldInfo(std::shared_ptr<NamedObject> field, const std::vector<size_t>& one_based_occurrences)
        :   m_field(field),
            m_fieldOccurrenceInfo(one_based_occurrences)
    {
    }

    long FieldInfo::Save(Log& log) const
    {
        Table& field_info_table = log.GetTable(ParadataTable::FieldInfo);
        long field_info_id = 0;
        field_info_table.Insert(&field_info_id,
            log.AddNamedObject(m_field),
            m_fieldOccurrenceInfo.Save(log)
        );

        return field_info_id;
    }

    // --------------------------------------------------------------


    void FieldValueInfo::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::FieldValueInfo)
                .AddColumn(_T("field_name"), Table::ColumnType::Long)
                .AddColumn(_T("special_type"), Table::ColumnType::Integer)
                        .AddCode((int)SpecialType::NotSpecial, _T("not_special"))
                        .AddCode((int)SpecialType::Notappl, _T("notappl"))
                        .AddCode((int)SpecialType::Missing, _T("missing"))
                        .AddCode((int)SpecialType::Default, _T("default"))
                        .AddCode((int)SpecialType::Refused, _T("refused"))
                .AddColumn(_T("value"), Table::ColumnType::Text)
                .AddIndex(_T("field_value_index"), { 0, 2 });
            ;
    }

    FieldValueInfo::FieldValueInfo(std::shared_ptr<NamedObject> field, SpecialType special_type, const CString& value)
        :   m_field(field),
            m_specialType(special_type),
            m_value(value)
    {
    }

    long FieldValueInfo::Save(Log& log) const
    {
        Table& field_value_info_table = log.GetTable(ParadataTable::FieldValueInfo);
        long field_value_info_id = 0;
        field_value_info_table.Insert(&field_value_info_id,
            log.AddNamedObject(m_field),
            (int)m_specialType,
            (LPCTSTR)m_value
        );

        return field_value_info_id;
    }

    // --------------------------------------------------------------


    void FieldValidationInfo::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::FieldValidationInfo)
                .AddColumn(_T("field_name"), Table::ColumnType::Long)
                .AddColumn(_T("value_set_name"), Table::ColumnType::Long, true)
                .AddColumn(_T("notappl_allowed"), Table::ColumnType::Boolean)
                .AddColumn(_T("notappl_confirmation"), Table::ColumnType::Boolean)
                .AddColumn(_T("outofrange_allowed"), Table::ColumnType::Boolean)
                .AddColumn(_T("outofrange_confirmation"), Table::ColumnType::Boolean)
                .AddIndex(_T("field_validation_info_index"), { 0 });
            ;
    }

    FieldValidationInfo::FieldValidationInfo(std::shared_ptr<NamedObject> field, std::shared_ptr<NamedObject> value_set,
        bool notappl_allowed, bool notappl_confirmation, bool out_of_range_allowed, bool out_of_range_confirmation)
        :   m_field(field),
            m_valueSet(value_set),
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
            log.AddNamedObject(m_field),
            GetOptionalValueOrNull(log.AddNullableNamedObject(m_valueSet)),
            m_notapplAllowed,
            m_notapplConfirmation,
            m_outOfRangeAllowed,
            m_outOfRangeConfirmation
        );

        return field_validation_info_id;
    }

    // --------------------------------------------------------------


    void FieldEntryInstance::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::FieldEntryInstance)
                .AddColumn(_T("field_info"), Table::ColumnType::Long)
            ;
    }

    FieldEntryInstance::FieldEntryInstance(std::shared_ptr<FieldInfo> field_info)
        :   m_fieldInfo(field_info)
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
}
