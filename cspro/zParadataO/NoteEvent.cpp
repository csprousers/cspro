#include "stdafx.h"
#include "NoteEvent.h"


using namespace Paradata;


void NoteEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::NoteEvent)
            .AddColumn(_T("source"), Table::ColumnType::Integer)
                    .AddCode((int)Source::Interface, _T("interface"))
                    .AddCode((int)Source::EditNote, _T("editnote"))
                    .AddCode((int)Source::PutNote, _T("putnote"))
            .AddColumn(_T("symbol_name"), Table::ColumnType::Long)
            .AddColumn(_T("field_info"), Table::ColumnType::Long, true)
            .AddColumn(_T("operatorid_info"), Table::ColumnType::Long)
            .AddColumn(_T("note_text"), Table::ColumnType::Long, true)
            .AddColumn(_T("edit_duration"), Table::ColumnType::Double, true)
        ;
}


NoteEvent::NoteEvent(Source source, std::shared_ptr<NamedObject> symbol, std::shared_ptr<FieldInfo> field_info, const CString& operator_id)
    :   m_source(source),
        m_symbol(std::move(symbol)),
        m_fieldInfo(std::move(field_info)),
        m_operatorId(operator_id)
{
}


void NoteEvent::SetPostEditValues(std::optional<std::wstring> modified_note_text)
{
    m_modifiedNoteText = std::move(modified_note_text);

    if( m_source != Source::PutNote )
        m_editDuration = ::GetTimestamp() - this->GetTimestamp();
}


void NoteEvent::Save(Log& log, long base_event_id) const
{
    const std::optional<long> field_info_id = ( m_fieldInfo != nullptr ) ? std::make_optional(m_fieldInfo->Save(log)) :
                                                                           std::nullopt;

    Table& note_event_table = log.GetTable(ParadataTable::NoteEvent);
    note_event_table.Insert(&base_event_id,
        static_cast<int>(m_source),
        log.AddNamedObject(m_symbol),
        GetOptionalValueOrNull(field_info_id),
        log.AddOperatorIdInfo(m_operatorId),
        GetOptionalValueOrNull(log.AddNullableText(m_modifiedNoteText)),
        GetOptionalValueOrNull(m_editDuration)
    );
}
