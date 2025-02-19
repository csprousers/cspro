#include "stdafx.h"
#include "ImputeEvent.h"

using namespace Paradata;


void ImputeEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::ImputeEvent)
            .AddColumn("field_info", Table::ColumnType::Long)
            .AddColumn("initial_value", Table::ColumnType::Double)
            .AddColumn("imputed_value", Table::ColumnType::Double)
        ;
}


ImputeEvent::ImputeEvent(std::shared_ptr<FieldInfo> field_info, const double initial_value, const double imputed_value)
    :   m_fieldInfo(std::move(field_info)),
        m_initialValue(initial_value),
        m_imputedValue(imputed_value)
{
}


void ImputeEvent::Save(Log& log, long base_event_id) const
{
    Table& impute_event_table = log.GetTable(ParadataTable::ImputeEvent);
    impute_event_table.Insert(&base_event_id,
        m_fieldInfo->Save(log),
        m_initialValue,
        m_imputedValue
    );
}
