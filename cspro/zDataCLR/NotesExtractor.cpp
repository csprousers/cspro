#include "Stdafx.h"
#include "NotesExtractor.h"
#include <zToolsO/DateTime.h>
#include <zUtilO/FileExtensions.h>
#include <zDictO/DictionaryCreator.h>
#include <zCaseO/CaseItemHelpers.h>
#include <zCaseO/NoteSorter.h>


namespace
{
    const TCHAR* NamePrefix = _T("NOTES_");

    const TCHAR* FieldNameName = _T("NOTES_FIELD_NAME");
    const TCHAR* OperatorIdName = _T("NOTES_OPERATOR_ID");
    const TCHAR* TimestampName = _T("NOTES_TIMESTAMP");
    const TCHAR* YearName = _T("NOTES_YEAR");
    const TCHAR* MonthName = _T("NOTES_MONTH");
    const TCHAR* DayName = _T("NOTES_DAY");
    const TCHAR* HourName = _T("NOTES_HOUR");
    const TCHAR* MinuteName = _T("NOTES_MINUTE");
    const TCHAR* SecondName = _T("NOTES_SECOND");
    const TCHAR* RecordOccurrenceName = _T("NOTES_RECORD_OCC");
    const TCHAR* ItemOccurrenceName = _T("NOTES_ITEM_OCC");
    const TCHAR* SubitemOccurrenceName = _T("NOTES_SUBITEM_OCC");
    const TCHAR* NoteName = _T("NOTES_NOTE");

    const size_t MaxNotesPerCase = 500;

    std::unique_ptr<CDataDict> CreateNotesDictionary(const CDataDict& source_dictionary, const CString& notes_dictionary_filename)
    {
        DictionaryCreator dictionary_creator(source_dictionary, NamePrefix, _T("Notes"), MaxNotesPerCase);
        dictionary_creator.AddItem(FieldNameName, _T("Field Name"), ContentType::Alpha, 100)
                          .AddItem(OperatorIdName, _T("Operator ID"), ContentType::Alpha, 50)
                          .AddItem(TimestampName, _T("Timestamp (UNIX)"), ContentType::Numeric, 10)
                          .AddItem(YearName, _T("Year (Local)"), ContentType::Numeric, 4)
                          .AddItem(MonthName, _T("Month (Local)"), ContentType::Numeric, 2)
                          .AddItem(DayName, _T("Day (Local)"), ContentType::Numeric, 2)
                          .AddItem(HourName, _T("Hour (Local)"), ContentType::Numeric, 2)
                          .AddItem(MinuteName, _T("Minute (Local)"), ContentType::Numeric, 2)
                          .AddItem(SecondName, _T("Second (Local)"), ContentType::Numeric, 2)
                          .AddItem(RecordOccurrenceName, _T("Record Occurrence"), ContentType::Numeric, 5)
                          .AddItem(ItemOccurrenceName, _T("Item Occurrence"), ContentType::Numeric, 5)
                          .AddItem(SubitemOccurrenceName, _T("Subitem Occurrence"), ContentType::Numeric, 5)
                          .AddItem(NoteName, _T("Note"), ContentType::Alpha, 999);

        auto notes_dictionary = dictionary_creator.GetDictionary();

        notes_dictionary->Save(notes_dictionary_filename);

        return notes_dictionary;
    }


    struct IdLink
    {
        size_t level_number;
        const CaseItem* input_case_item;
        const CaseItem* notes_case_item;
    };

    std::vector<IdLink> GetIdLinks(const CaseAccess& input_case_access, const CaseAccess& notes_case_access)
    {
        std::vector<IdLink> id_links;

        for( const CDictItem* id_item : input_case_access.GetDataDict().GetIdItems() )
        {
            id_links.emplace_back(IdLink
                {
                    id_item->GetLevel()->GetLevelNumber(),
                    input_case_access.LookupCaseItem(*id_item),
                    notes_case_access.LookupCaseItem(NamePrefix + id_item->GetName())
                });
        }

        return id_links;
    };


    struct NoteLinks
    {
        const CaseItem* field_name_case_item;
        const CaseItem* operator_id_case_item;
        const CaseItem* timestamp_case_item;
        const CaseItem* year_case_item;
        const CaseItem* month_case_item;
        const CaseItem* day_case_item;
        const CaseItem* hour_case_item;
        const CaseItem* minute_case_item;
        const CaseItem* second_case_item;
        const CaseItem* record_occurrence_case_item;
        const CaseItem* item_occurrence_case_item;
        const CaseItem* subitem_occurrence_case_item;
        const CaseItem* note_case_item;
    };

    NoteLinks GetNoteLinks(const CaseAccess& notes_case_access)
    {
        return NoteLinks
        {
            notes_case_access.LookupCaseItem(FieldNameName),
            notes_case_access.LookupCaseItem(OperatorIdName),
            notes_case_access.LookupCaseItem(TimestampName),
            notes_case_access.LookupCaseItem(YearName),
            notes_case_access.LookupCaseItem(MonthName),
            notes_case_access.LookupCaseItem(DayName),
            notes_case_access.LookupCaseItem(HourName),
            notes_case_access.LookupCaseItem(MinuteName),
            notes_case_access.LookupCaseItem(SecondName),
            notes_case_access.LookupCaseItem(RecordOccurrenceName),
            notes_case_access.LookupCaseItem(ItemOccurrenceName),
            notes_case_access.LookupCaseItem(SubitemOccurrenceName),
            notes_case_access.LookupCaseItem(NoteName)
        };
    }

    const CaseLevel* FindCaseLevel(Case& data_case, const CString& level_key)
    {
        auto input_case_levels = data_case.GetAllCaseLevels();

        const auto& case_level_search = std::find_if(input_case_levels.cbegin(), input_case_levels.cend(),
            [&](const CaseLevel* case_level) { return ( case_level->GetLevelKey().Compare(level_key) == 0 ); });

        return ( case_level_search == input_case_levels.cend() ) ? nullptr : *case_level_search;
    }
}


System::String^ CSPro::Data::NotesExtractor::Extract(System::ComponentModel::BackgroundWorker^ background_worker,
    CSPro::Data::DataRepository^ input_repository, CSPro::Util::ConnectionString^ notes_connection_string)
{
    try
    {
        auto native_input_repository = input_repository->GetNativePointer();

        auto input_case_access = native_input_repository->GetCaseAccess();
        auto input_case = input_case_access->CreateCase();

        // create a notes dictionary
        CString notes_dictionary_filename = CString(System::IO::Path::Combine(System::IO::Path::GetDirectoryName(notes_connection_string->Filename),
                                                                              System::IO::Path::GetFileNameWithoutExtension(notes_connection_string->Filename))) +
                                                    FileExtensions::WithDot::Dictionary;
       
        auto notes_dictionary = CreateNotesDictionary(input_case_access->GetDataDict(), notes_dictionary_filename);

        // create a repository using the notes dictionary
        std::shared_ptr<CaseAccess> notes_case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*notes_dictionary);
        std::unique_ptr<Case> notes_case = notes_case_access->CreateCase();

        std::unique_ptr<::DataRepository> notes_repository(::DataRepository::CreateAndOpen(notes_case_access,
            notes_connection_string->GetNativeConnectionString(), DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew));

        // link the IDs and the note fields
        const auto& id_links = GetIdLinks(*input_case_access, *notes_case_access);
        const auto& note_links = GetNoteLinks(*notes_case_access);

        // process the cases
        size_t total_cases = native_input_repository->GetNumberCases();
        size_t cases_read = 0;
        size_t extracted_notes = 0;

        const size_t ProgressUpdateFrequency = 250;
        size_t cases_until_progress_update = ProgressUpdateFrequency;

        auto case_iterator = native_input_repository->CreateCaseIterator(::CaseIterationMethod::SequentialOrder, ::CaseIterationOrder::Ascending);

        while( case_iterator->NextCase(*input_case) )
        {
            ++cases_read;

            auto notes = GetSortedNotes(*input_case);

            if( !notes.empty() )
            {
                notes_case->Reset();

                // copy the root IDs
                auto input_id_index = input_case->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();
                auto notes_id_index = notes_case->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();

                for( const auto& id_link : id_links )
                {
                    if( id_link.level_number == 0 )
                        CaseItemHelpers::CopyValue(*id_link.input_case_item, input_id_index, *id_link.notes_case_item, notes_id_index);
                }

                // copy the notes
                CaseRecord& notes_case_record = notes_case->GetRootCaseLevel().GetCaseRecord(0);
                size_t note_occurrences = std::min(MaxNotesPerCase, notes.size());
                notes_case_record.SetNumberOccurrences(note_occurrences);
                auto notes_index = notes_case_record.GetCaseItemIndex();

                for( const auto& note : notes )
                {
                    // if on a level, set the IDs for that level
                    if( !note->GetNamedReference().GetLevelKey().IsEmpty() )
                    {
                        const CaseLevel* case_level = FindCaseLevel(*input_case, note->GetNamedReference().GetLevelKey());

                        if( case_level != nullptr )
                        {
                            // copy over the level keys
                            for( size_t level_number = 1; level_number <= case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber(); ++level_number )
                            {
                                const CaseLevel* this_case_level = case_level;

                                while( this_case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() != level_number )
                                    this_case_level = &this_case_level->GetParentCaseLevel();

                                for( const auto& id_link : id_links )
                                {
                                    if( id_link.level_number == level_number )
                                    {
                                        CaseItemHelpers::CopyValue(*id_link.input_case_item,
                                            this_case_level->GetIdCaseRecord().GetCaseItemIndex(), *id_link.notes_case_item, notes_index);
                                    }
                                }
                            }
                        }
                    }

                    CaseItemHelpers::SetValue(*note_links.field_name_case_item, notes_index, note->GetNamedReference().GetName());
                    CaseItemHelpers::SetValue(*note_links.operator_id_case_item, notes_index, note->GetOperatorId());
                    CaseItemHelpers::SetValue(*note_links.timestamp_case_item, notes_index, (double)note->GetModifiedDateTime());

                    // convert the timestamp to local time
                    int year, month, day, hour, minute, second;
                    time_t tm = note->GetModifiedDateTime();
                    TmToReadableTime(localtime(&tm), &year, &month, &day, &hour, &minute, &second);

                    CaseItemHelpers::SetValue(*note_links.year_case_item, notes_index, year);
                    CaseItemHelpers::SetValue(*note_links.month_case_item, notes_index, month);
                    CaseItemHelpers::SetValue(*note_links.day_case_item, notes_index, day);
                    CaseItemHelpers::SetValue(*note_links.hour_case_item, notes_index, hour);
                    CaseItemHelpers::SetValue(*note_links.minute_case_item, notes_index, minute);
                    CaseItemHelpers::SetValue(*note_links.second_case_item, notes_index, second);

                    if( note->GetNamedReference().HasOccurrences() )
                    {
                        const auto& one_based_occurrences = note->GetNamedReference().GetOneBasedOccurrences();
                        ASSERT(one_based_occurrences.size() == 3);

                        if( one_based_occurrences[0] > 0 )
                            CaseItemHelpers::SetValue(*note_links.record_occurrence_case_item, notes_index, one_based_occurrences[0]);

                        if( one_based_occurrences[1] > 0 )
                            CaseItemHelpers::SetValue(*note_links.item_occurrence_case_item, notes_index, one_based_occurrences[1]);

                        if( one_based_occurrences[2] > 0 )
                            CaseItemHelpers::SetValue(*note_links.subitem_occurrence_case_item, notes_index, one_based_occurrences[2]);
                    }

                    CaseItemHelpers::SetValue(*note_links.note_case_item, notes_index, note->GetContent());

                    ++extracted_notes;

                    notes_index.IncrementRecordOccurrence();

                    if( notes_index.GetRecordOccurrence() == note_occurrences )
                        break;
                }

                // save the notes case
                notes_repository->WriteCase(*notes_case);
            }
            

            if( --cases_until_progress_update == 0 )
            {
                // update progress bar
                background_worker->ReportProgress(case_iterator->GetPercentRead(),
                    System::String::Format("Case {0} of {1} [{2}]", cases_read, total_cases, gcnew System::String(input_case->GetKey())));

                cases_until_progress_update = ProgressUpdateFrequency;
            }

            // if the operation is cancelled, delete the notes repository
            if( background_worker->CancellationPending )
            {
                notes_repository->DeleteRepository();
                return nullptr;
            }
        }

        background_worker->ReportProgress(100, gcnew System::String("Extraction complete"));

        System::String^ message = System::String::Format("Successfully extracted {0} notes.", extracted_notes);

        return message;
    }

    catch( const DataRepositoryException::Error& exception )
    {
        return gcnew System::String(exception.GetErrorMessage().c_str());
    }
}
