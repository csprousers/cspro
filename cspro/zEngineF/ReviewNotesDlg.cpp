#include "StdAfx.h"
#include "ReviewNotesDlg.h"


CREATE_JSON_KEY(canGoto)
CREATE_JSON_KEY(gotoNote)
CREATE_JSON_KEY(groupLabel)
CREATE_JSON_KEY(noteGroups)
   

ReviewNotesDlg::ReviewNotesDlg()
    :   m_gotoNote(nullptr)
{
}


std::string ReviewNotesDlg::GetDialogName()
{
    return "note-review";
}


SharableString ReviewNotesDlg::GetJsonArgumentsText()
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    // note groups
    json_writer->WriteObjects(JK::noteGroups, m_groupedReviewNotes,
        [&](const std::vector<const ReviewNote*>& grouped_notes)
        {
            json_writer->Write(JK::groupLabel, grouped_notes.front()->group_label);

            // notes
            json_writer->WriteObjects(JK::notes, grouped_notes,
                [&](const ReviewNote* const review_note)
                {
                    const uint64_t note_index = m_reviewNoteIndexMap.size() + 1;
                    m_reviewNoteIndexMap[note_index] = review_note;

                    json_writer->Write(JK::index, note_index)
                                .Write(JK::canGoto, review_note->can_goto)
                                .Write(JK::label, review_note->label)
                                .Write(JK::content, review_note->content);
                });
        });

    json_writer->EndObject();

    return json_writer->ReleaseSharableString();
}


void ReviewNotesDlg::ProcessJsonResults(const JsonNode& json_results)
{
    auto get_review_note = [&](const uint64_t note_index) -> const ReviewNote&
    {
        const auto& review_note_lookup = m_reviewNoteIndexMap.find(note_index);

        if( review_note_lookup == m_reviewNoteIndexMap.cend() )
            throw CSProException("Invalid note index: "+ IntToString(note_index));

        return *review_note_lookup->second;
    };

    // get the deleted notes
    if( json_results.Contains(JK::deleted) )
    {
        for( const JsonNode& deleted_element : json_results.GetArray(JK::deleted) )
        {
            const ReviewNote& review_note = get_review_note(deleted_element.Get<uint64_t>());
            m_deletedNotes.insert(&review_note.note);
        }
    }

    // see if there is a note to go to
    if( json_results.Contains(JK::gotoNote) )
    {
        const ReviewNote& review_note = get_review_note(json_results.Get<uint64_t>(JK::gotoNote));

        if( !review_note.can_goto )
            throw CSProException("You can only go to field notes.");

        m_gotoNote = &review_note.note;

        if( m_deletedNotes.find(m_gotoNote) != m_deletedNotes.cend() )
            throw CSProException("You cannot go to a deleted note.");
    }
}
