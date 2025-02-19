#pragma once

#include <zParadataO/Event.h>

namespace Paradata { class CaseEvent; class FieldInfo; class NoteEvent; }


class ZPARADATAO_API Paradata::NoteEvent : public Event
{
    DECLARE_PARADATA_EVENT(NoteEvent)

public:
    enum class Source
    {
        Interface,
        EditNote,
        PutNote
    };

public:
    NoteEvent(Source source, std::shared_ptr<NamedObject> symbol, std::shared_ptr<FieldInfo> field_info, std::string operator_id);

    void SetPostEditValues(SharableString modified_note_text);

private:
    const Source m_source;
    const std::shared_ptr<NamedObject> m_symbol;
    const std::shared_ptr<const FieldInfo> m_fieldInfo;
    const std::string m_operatorId;
    SharableString m_modifiedNoteText;
    std::optional<double> m_editDuration;
};
