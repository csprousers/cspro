#include "stdafx.h"
#include "PropertyEvent.h"

using namespace Paradata;


void PropertyEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::PropertyInfo)
            .AddColumn("property", Table::ColumnType::Text)
            .AddColumn("value", Table::ColumnType::Text)
            .AddIndex("property_info_index", { 0 })
        ;

    log.CreateTable(ParadataTable::PropertyEvent)
            .AddColumn("property_info", Table::ColumnType::Long)
            .AddColumn("type", Table::ColumnType::Boolean)
                    .AddCode(0, "initial")
                    .AddCode(1, "user_modified")
            .AddColumn("item_name", Table::ColumnType::Long, true)
        ;
}


PropertyEvent::PropertyEvent(std::string property, std::string value, bool user_modified, std::shared_ptr<NamedObject> dict_item/* = nullptr*/)
    :   m_property(std::move(property)),
        m_value(std::move(value)),
        m_userModified(user_modified),
        m_dictItem(std::move(dict_item))
{
}


void PropertyEvent::Save(Log& log, long base_event_id) const
{
    // fill the property info table
    Table& property_info_table = log.GetTable(ParadataTable::PropertyInfo);
    long property_info_id = 0;
    property_info_table.Insert(&property_info_id,
        m_property.c_str(),
        m_value.c_str()
    );

    // fill the property event table
    Table& property_event_table = log.GetTable(ParadataTable::PropertyEvent);
    property_event_table.Insert(&base_event_id,
        property_info_id,
        m_userModified,
        GetOptionalValueOrNull(log.AddNullableNamedObject(m_dictItem.get()))
    );
}
