#pragma once

#include <zParadataO/Event.h>
#include <zParadataO/FieldInfo.h>


namespace Paradata
{
    class ZPARADATAO_API NoteEvent : public Event
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
        NoteEvent(Source source, std::shared_ptr<NamedObject> symbol, std::shared_ptr<FieldInfo> field_info, const CString& operator_id);

        void SetPostEditValues(std::optional<std::wstring> modified_note_text);

    private:
        Source m_source;
        std::shared_ptr<NamedObject> m_symbol;
        std::shared_ptr<FieldInfo> m_fieldInfo;
        CString m_operatorId;
        std::optional<std::wstring> m_modifiedNoteText;
        std::optional<double> m_editDuration;
    };
}
