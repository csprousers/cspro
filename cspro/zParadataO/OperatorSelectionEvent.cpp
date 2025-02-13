#include "stdafx.h"
#include "OperatorSelectionEvent.h"

using namespace Paradata;


void OperatorSelectionEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::OperatorSelectionEvent)
            .AddColumn(_T("source"), Table::ColumnType::Integer)
                    .AddCode((int)Source::Errmsg, _T("errmsg"))
                    .AddCode((int)Source::Accept, _T("accept"))
                    .AddCode((int)Source::Prompt, _T("prompt"))
                    .AddCode((int)Source::Userbar, _T("userbar"))
                    .AddCode((int)Source::SelCase, _T("selcase"))
                    .AddCode((int)Source::Show, _T("show"))
                    .AddCode((int)Source::ShowArray, _T("showarray"))
                    .AddCode((int)Source::ListShow, _T("list.show"))
                    .AddCode((int)Source::MapShow, _T("map.show"))
                    .AddCode((int)Source::ValueSetShow, _T("valueset.show"))
                    .AddCode((int)Source::BarcodeRead, _T("barcode.read"))
            .AddColumn(_T("selection_number"), Table::ColumnType::Integer, true)
            .AddColumn(_T("selection_text"), Table::ColumnType::Long, true)
            .AddColumn(_T("display_duration"), Table::ColumnType::Double, true)
        ;
}


OperatorSelectionEvent::OperatorSelectionEvent(Source source)
    :   m_source(source)
{
}


void OperatorSelectionEvent::SetPostSelectionValues(std::optional<int> selection_number, std::optional<std::wstring> selection_text, bool set_display_duration)
{
    m_selectionNumber = std::move(selection_number);
    m_selectionText = std::move(selection_text);

    if( set_display_duration )
        m_displayDuration = ::GetTimestamp() - this->GetTimestamp();
}


void OperatorSelectionEvent::Save(Log& log, long base_event_id) const
{
    Table& operator_selection_event_table = log.GetTable(ParadataTable::OperatorSelectionEvent);
    operator_selection_event_table.Insert(&base_event_id,
        static_cast<int>(m_source),
        GetOptionalValueOrNull(m_selectionNumber),
        GetOptionalValueOrNull(log.AddNullableText(m_selectionText)),
        GetOptionalValueOrNull(m_displayDuration)
    );
}
