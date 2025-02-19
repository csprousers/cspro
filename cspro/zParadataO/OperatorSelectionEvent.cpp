#include "stdafx.h"
#include "OperatorSelectionEvent.h"

using namespace Paradata;


void OperatorSelectionEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::OperatorSelectionEvent)
            .AddColumn("source", Table::ColumnType::Integer)
                    .AddCode(Source::Errmsg, "errmsg")
                    .AddCode(Source::Accept, "accept")
                    .AddCode(Source::Prompt, "prompt")
                    .AddCode(Source::Userbar, "userbar")
                    .AddCode(Source::SelCase, "selcase")
                    .AddCode(Source::Show, "show")
                    .AddCode(Source::ShowArray, "showarray")
                    .AddCode(Source::ListShow, "List.show")
                    .AddCode(Source::MapShow, "Map.show")
                    .AddCode(Source::ValueSetShow, "ValueSet.show")
                    .AddCode(Source::BarcodeRead, "Barcode.read")
            .AddColumn("selection_number", Table::ColumnType::Integer, true)
            .AddColumn("selection_text", Table::ColumnType::Long, true)
            .AddColumn("display_duration", Table::ColumnType::Double, true)
        ;
}


OperatorSelectionEvent::OperatorSelectionEvent(const Source source)
    :   m_source(source)
{
}


void OperatorSelectionEvent::SetPostSelectionValues(std::optional<int> selection_number, SharableString selection_text, const bool set_display_duration)
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
