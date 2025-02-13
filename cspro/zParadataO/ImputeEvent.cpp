#include "stdafx.h"
#include "ImputeEvent.h"

namespace Paradata
{
    void ImputeEvent::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::ImputeEvent)
                .AddColumn(_T("field_info"), Table::ColumnType::Long)
                .AddColumn(_T("initial_value"), Table::ColumnType::Double)
                .AddColumn(_T("imputed_value"), Table::ColumnType::Double)
            ;
    }

    ImputeEvent::ImputeEvent(std::shared_ptr<FieldInfo> field_info, double initial_value, double imputed_value)
        :   m_fieldInfo(field_info),
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
}
