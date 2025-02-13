#include "stdafx.h"
#include "PropertyEvent.h"

namespace Paradata
{
    void PropertyEvent::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::PropertyInfo)
                .AddColumn(_T("property"), Table::ColumnType::Text)
                .AddColumn(_T("value"), Table::ColumnType::Text)
                .AddIndex(_T("property_info_index"), { 0 })
            ;

        log.CreateTable(ParadataTable::PropertyEvent)
                .AddColumn(_T("property_info"), Table::ColumnType::Long)
                .AddColumn(_T("type"), Table::ColumnType::Boolean)
                        .AddCode(0, _T("initial"))
                        .AddCode(1, _T("user_modified"))
                .AddColumn(_T("item_name"), Table::ColumnType::Long, true)
            ;
    }

    PropertyEvent::PropertyEvent(const CString& property, const CString& value,
        bool user_modified, std::shared_ptr<NamedObject> dictionary_item/* = nullptr*/)
        :   m_property(property),
            m_value(value),
            m_userModified(user_modified),
            m_dictionaryItem(dictionary_item)
    {
    }

    void PropertyEvent::Save(Log& log, long base_event_id) const
    {
        // fill the property info table
        Table& property_info_table = log.GetTable(ParadataTable::PropertyInfo);
        long property_info_id = 0;
        property_info_table.Insert(&property_info_id,
            (LPCTSTR)m_property,
            (LPCTSTR)m_value
        );

        // fill the property event table
        Table& property_event_table = log.GetTable(ParadataTable::PropertyEvent);
        property_event_table.Insert(&base_event_id,
            property_info_id,
            m_userModified,
            GetOptionalValueOrNull(log.AddNullableNamedObject(m_dictionaryItem))
        );
    }
}
