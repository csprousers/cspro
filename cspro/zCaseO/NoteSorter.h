#pragma once

#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/Note.h>


inline std::vector<const Note*> GetSortedNotes(const Case& data_case)
{
    using nwsi = std::tuple<const Note*, std::string>;
    std::vector<nwsi> notes_with_sort_index;

    for( const Note& note : data_case.GetNotes() )
    {
        std::string sort_index;

        const NamedReference& named_reference = note.GetNamedReference();
        const CaseItemReference* const case_item_reference = dynamic_cast<const CaseItemReference*>(&named_reference);

        // sort the case note first, then any non-field notes (alphabetically), and then any field notes in dictionary order
        if( case_item_reference == nullptr )
        {
            if( named_reference.GetName() == data_case.GetCaseMetadata().GetDictionary().GetName() )
            {
                sort_index = "!";
            }

            else
            {
                sort_index = "#" + named_reference.GetName();
            }
        }

        else
        {
            size_t level_index = 0;

            // get the index of this level
            if( !named_reference.GetLevelKey().empty() )
            {
                const std::vector<const CaseLevel*> case_levels = data_case.GetAllCaseLevels();

                for( level_index = 1; level_index < case_levels.size(); ++level_index )
                {
                    if( UTF8_TODO::GetUtf8(case_levels[level_index]->GetLevelKey()) == named_reference.GetLevelKey() )
                        break;
                }
            }

            const CDictItem& dict_item = case_item_reference->GetCaseItem().GetDictItem();
            sort_index = FormatText("%d%d%d", static_cast<int>(level_index), dict_item.GetRecord()->GetSonNumber(), dict_item.GetSonNumber());
        }

        // add the time to the sort index
        sort_index.append(IntToString(static_cast<int64_t>(note.GetModifiedDateTime())));

        notes_with_sort_index.emplace_back(&note, std::move(sort_index));
    }

    std::sort(notes_with_sort_index.begin(), notes_with_sort_index.end(),
        [](const nwsi& nwsi1, const nwsi& nwsi2)
        {
            return ( std::get<1>(nwsi1) < std::get<1>(nwsi2) );
        });

    std::vector<const Note*> sorted_notes;

    for( const nwsi& note_with_sort_index : notes_with_sort_index )
        sorted_notes.emplace_back(std::get<0>(note_with_sort_index));

    return sorted_notes;
}
