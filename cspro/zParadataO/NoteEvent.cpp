#include "stdafx.h"
#include "NoteEvent.h"

using namespace Paradata;


void NoteEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::NoteEvent)
            .AddColumn("source", Table::ColumnType::Integer)
                    .AddCode(Source::Interface, "interface")
                    .AddCode(Source::EditNote, "editnote")
                    .AddCode(Source::PutNote, "putnote")
            .AddColumn("symbol_name", Table::ColumnType::Long)
            .AddColumn("field_info", Table::ColumnType::Long, true)
            .AddColumn("operatorid_info", Table::ColumnType::Long)
            .AddColumn("note_text", Table::ColumnType::Long, true)
            .AddColumn("edit_duration", Table::ColumnType::Double, true)
        ;
}


NoteEvent::NoteEvent(const Source source, std::shared_ptr<NamedObject> symbol, std::shared_ptr<FieldInfo> field_info, std::string operator_id)
    :   m_source(source),
        m_symbol(std::move(symbol)),
        m_fieldInfo(std::move(field_info)),
        m_operatorId(std::move(operator_id))
{
}


void NoteEvent::SetPostEditValues(SharableString modified_note_text)
{
    m_modifiedNoteText = std::move(modified_note_text);

    if( m_source != Source::PutNote )
        m_editDuration = ::GetTimestamp<double>() - this->GetTimestamp();
}


void NoteEvent::Save(Log& log, long base_event_id) const
{
    const std::optional<long> field_info_id = ( m_fieldInfo != nullptr ) ? std::make_optional(m_fieldInfo->Save(log)) :
                                                                           std::nullopt;

    Table& note_event_table = log.GetTable(ParadataTable::NoteEvent);
    note_event_table.Insert(&base_event_id,
        static_cast<int>(m_source),
        log.AddNamedObject(m_symbol.get()),
        GetOptionalValueOrNull(field_info_id),
        log.AddOperatorIdInfo(m_operatorId),
        GetOptionalValueOrNull(log.AddNullableText(m_modifiedNoteText)),
        GetOptionalValueOrNull(m_editDuration)
    );
}
