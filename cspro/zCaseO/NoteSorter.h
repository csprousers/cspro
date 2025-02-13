#pragma once

#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/Note.h>
#include <zToolsO/Tools.h>


std::vector<const Note*> GetSortedNotes(const Case& data_case)
{
    using nwsi = std::tuple<const Note*, CString>;
    std::vector<nwsi> notes_with_sort_index;

    for( const auto& note : data_case.GetNotes() )
    {
        CString sort_index;

        const auto& named_reference = note.GetNamedReference();
        const auto case_item_reference = dynamic_cast<const CaseItemReference*>(&named_reference);

        // sort the case note first, then any non-field notes (alphabetically), and then any field notes in dictionary order
        if( case_item_reference == nullptr )
        {
            if( named_reference.GetName().Compare(data_case.GetCaseMetadata().GetDictionary().GetName()) == 0 )
                sort_index = _T("!");

            else
                sort_index.Format(_T("#%s"), (LPCTSTR)named_reference.GetName());
        }

        else
        {
            size_t level_index = 0;

            // get the index of this level
            if( !named_reference.GetLevelKey().IsEmpty() )
            {
                const auto& case_levels = data_case.GetAllCaseLevels();

                for( level_index = 1; level_index < case_levels.size(); ++level_index )
                {
                    if( case_levels[level_index]->GetLevelKey().Compare(named_reference.GetLevelKey()) == 0 )
                        break;
                }
            }

            const CDictItem& dictionary_item = case_item_reference->GetCaseItem().GetDictionaryItem();
            sort_index.Format(_T("%d%d%d"), (int)level_index, dictionary_item.GetRecord()->GetSonNumber(), dictionary_item.GetSonNumber());
        }

        // add the time to the sort index
        sort_index.Append(IntToString((int)note.GetModifiedDateTime()));

        notes_with_sort_index.emplace_back(&note, sort_index);
    }

    std::sort(notes_with_sort_index.begin(), notes_with_sort_index.end(), [](const nwsi& nwsi1, const nwsi& nwsi2)
    {
        return ( std::get<1>(nwsi1).Compare(std::get<1>(nwsi2)) < 0 );
    });

    std::vector<const Note*> sorted_notes;

    for( const auto& note_with_sort_index : notes_with_sort_index )
        sorted_notes.emplace_back(std::get<0>(note_with_sort_index));

    return sorted_notes;
}
