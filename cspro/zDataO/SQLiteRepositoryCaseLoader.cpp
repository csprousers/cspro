#include "stdafx.h"
#include "SQLiteRepository.h"
#include "ISQLiteQuestionnaireSerializer.h"
#include <zCaseO/CaseConstructionHelpers.h>


bool SQLiteRepository::ReadCaseFromDatabase(Case& data_case, SQLiteStatement& get_case_statement)
{
    if( get_case_statement.Step() != SQLITE_ROW )
        return false;

    data_case.Reset();

    int column = 0;

    data_case.SetUuid(get_case_statement.GetColumn<std::wstring>(column++));

    data_case.SetPositionInRepository(get_case_statement.GetColumn<double>(column++));

    data_case.SetDeleted(get_case_statement.GetColumn<int>(column++) != 0);

    if( m_caseAccess->GetUsesCaseLabels() )
        data_case.SetCaseLabel(get_case_statement.GetColumn<CString>(column++));

    if( m_caseAccess->GetUsesStatuses() )
    {
        data_case.SetVerified(get_case_statement.GetColumn<int>(column++) != 0);

        // read the partial save information
        if( get_case_statement.IsColumnNull(column) )
        {
            data_case.SetPartialSaveMode(PartialSaveMode::None);
            column += 6;
        }

        else
        {
            const int partial_save_mode_int = get_case_statement.GetColumn<int>(column++);
            ASSERT(partial_save_mode_int >= static_cast<int>(PartialSaveMode::Add) && partial_save_mode_int <= static_cast<int>(PartialSaveMode::Verify));

            std::shared_ptr<CaseItemReference> partial_save_case_item_reference;

            if( get_case_statement.IsColumnNull(column) )
            {
                column += 5;
            }

            else
            {
                CString field_name = get_case_statement.GetColumn<CString>(column++);
                CString level_key = get_case_statement.GetColumn<CString>(column++);

                size_t occurrences[3];
                for( size_t i = 0; i < _countof(occurrences); ++i )
                    occurrences[i] = std::max(get_case_statement.GetColumn<int>(column++) - 1, 0);

                partial_save_case_item_reference = CaseConstructionHelpers::CreateCaseItemReference(*m_caseAccess, level_key, field_name, occurrences);
            }

            data_case.SetPartialSaveStatus((PartialSaveMode)partial_save_mode_int, std::move(partial_save_case_item_reference));
        }
    }

    // read the notes
    if( m_caseAccess->GetUsesNotes() )
    {
        std::vector<Note>& notes = data_case.GetNotes();
        notes.clear();

        SQLiteStatement get_notes_statement(m_stmtGetNotes);
        get_notes_statement.Bind(1, data_case.GetUuid());

        while( get_notes_statement.Step() == SQLITE_ROW )
        {
            CString field_name = get_notes_statement.GetColumn<CString>(0);
            CString level_key = get_notes_statement.GetColumn<CString>(1);

            size_t occurrences[3];
            for( size_t i = 0; i < _countof(occurrences); ++i )
                occurrences[i] = std::max(get_notes_statement.GetColumn<int>(i + 2) - 1, 0);

            CString content = get_notes_statement.GetColumn<CString>(5);
            CString operator_id = get_notes_statement.GetColumn<CString>(6);
            time_t modified_date_time = (time_t)get_notes_statement.GetColumn<int64_t>(7);

            std::shared_ptr<NamedReference> named_reference = CaseConstructionHelpers::CreateNamedReference(*m_caseAccess, level_key, field_name, occurrences);

            notes.emplace_back(content, std::move(named_reference), operator_id, modified_date_time);
        }
    }

    // read the vector clock
    VectorClock& vector_clock = data_case.GetVectorClock();
    vector_clock.clear();

    if( m_stmtGetClock != nullptr )
    {
        SQLiteStatement get_clock_statement(m_stmtGetClock);
        get_clock_statement.Bind(1, data_case.GetUuid());

        // read the device and revision
        while( get_clock_statement.Step() == SQLITE_ROW )
            vector_clock.setVersion(get_clock_statement.GetColumn<CString>(0), get_clock_statement.GetColumn<int>(1));
    }

    // Read the questionnaire
    m_questionnaireSerializer->ReadQuestionnaire(data_case);

    return true;
}
