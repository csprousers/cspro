#include "StdAfx.h"
#include "ExtractNotesTask.h"
#include "TaskRunner.h"
#include <zDictO/DictionaryCreator.h>
#include <zCaseO/CaseItemHelpers.h>
#include <zCaseO/NoteSorter.h>


CREATE_ENUM_JSON_SERIALIZER(ExtractNotesSettings::OutputType,
    { ExtractNotesSettings::OutputType::CSPro, "CSPro" },
    { ExtractNotesSettings::OutputType::CSV,   "CSV" },
    { ExtractNotesSettings::OutputType::Excel, "Excel" })


// --------------------------------------------------------------------------
// ExtractNotesSettings
// --------------------------------------------------------------------------

ExtractNotesSettings ExtractNotesSettings::CreateFromJson(const JsonNode& json_node)
{
    return ExtractNotesSettings
    {
        json_node.Get<OutputType>(JK::format),
        json_node.Get<ConnectionString>(JK::notes),
        json_node.GetOrConstruct<std::string>(JK::dictionary)
    };
}


void ExtractNotesSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::format, output_type)
               .Write(JK::notes, notes_connection_string)
               .WriteIfNotBlank(JK::dictionary, notes_dictionary_file_path)
               .EndObject();
}



// --------------------------------------------------------------------------
// ExtractNotesTask
// --------------------------------------------------------------------------

ExtractNotesTask::ExtractNotesTask(ExtractNotesSettings settings)
    :   m_settings(std::move(settings)),
        m_casesWithNotes(0),
        m_extractedNotes(0)
{
}


ExtractNotesTask::~ExtractNotesTask()
{
}


void ExtractNotesTask::Initialize()
{
    m_taskRunner->SetTitle("Extracting notes...");

    m_notesDictionary = CreateNotesDictionary(m_caseAccess->GetDataDict());

    if( !m_settings.notes_dictionary_file_path.empty() )
    {
        ASSERT(m_settings.output_type == ExtractNotesSettings::OutputType::CSPro);
        m_taskRunner->LogText("Saving notes dictionary: " + m_settings.notes_dictionary_file_path);
        m_notesDictionary->Save(m_settings.notes_dictionary_file_path);
    }

    m_notesCaseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(*m_notesDictionary);

    m_notesDataRepository = DataRepository::CreateAndOpen(m_notesCaseAccess,
                                                          m_settings.notes_connection_string,
                                                          DataRepositoryAccess::BatchOutput,
                                                          DataRepositoryOpenFlag::CreateNew);

    m_taskRunner->LogText("Saving notes to data source: " + m_notesDataRepository->GetName(DataRepositoryNameType::Full));
    m_taskRunner->LogText();

    m_notesCase = m_notesCaseAccess->CreateCase();
    m_notesCase->SetCaseConstructionReporter(m_taskRunner->GetCaseConstructionReporter(*m_notesCaseAccess));

    SetUpIdLinks();
    SetUpNoteLinks();
}


void ExtractNotesTask::ProcessCase(Case& data_case)
{
    if( data_case.GetNotes().empty() )
        return;

    ++m_casesWithNotes;

    CopyNotesToCase(data_case, GetSortedNotes(data_case));

    m_notesDataRepository->WriteCase(*m_notesCase);
}


void ExtractNotesTask::Finalize(const Result result)
{
    if( result == Result::Complete )
    {
        m_taskRunner->LogText("Process summary:");
        m_taskRunner->LogText("    Cases processed: %d", static_cast<int>(GetCasesProcessed()));
        m_taskRunner->LogText("    Cases with notes: %d", static_cast<int>(m_casesWithNotes));

        m_taskRunner->LogText();
        m_taskRunner->LogText("Successfully extracted %d note%s.", static_cast<int>(m_extractedNotes),
                                                                   PluralizeWord(m_extractedNotes));
    }

    else if( m_notesDataRepository != nullptr )
    {
        m_taskRunner->LogText();
        m_taskRunner->LogText("Deleting the data source: " + m_notesDataRepository->GetName(DataRepositoryNameType::Full));
        m_notesDataRepository->DeleteRepository();
    }

    m_notesDataRepository.reset();
}


namespace
{
    constexpr const char* FieldNameName         = "NOTES_FIELD_NAME";
    constexpr const char* OperatorIdName        = "NOTES_OPERATOR_ID";
    constexpr const char* TimestampName         = "NOTES_TIMESTAMP";
    constexpr const char* YearName              = "NOTES_YEAR";
    constexpr const char* MonthName             = "NOTES_MONTH";
    constexpr const char* DayName               = "NOTES_DAY";
    constexpr const char* HourName              = "NOTES_HOUR";
    constexpr const char* MinuteName            = "NOTES_MINUTE";
    constexpr const char* SecondName            = "NOTES_SECOND";
    constexpr const char* RecordOccurrenceName  = "NOTES_RECORD_OCC";
    constexpr const char* ItemOccurrenceName    = "NOTES_ITEM_OCC";
    constexpr const char* SubitemOccurrenceName = "NOTES_SUBITEM_OCC";
    constexpr const char* NoteName              = "NOTES_NOTE";

    const size_t MaxNotesPerCase = 500;
}


std::unique_ptr<CDataDict> ExtractNotesTask::CreateNotesDictionary(const CDataDict& source_dictionary)
{
    DictionaryCreator dictionary_creator(source_dictionary, NamePrefix, "Notes", MaxNotesPerCase);

    dictionary_creator.AddItem(FieldNameName, "Field Name", ContentType::Alpha, 100)
                      .AddItem(OperatorIdName, "Operator ID", ContentType::Alpha, 50)
                      .AddItem(TimestampName, "Timestamp (UNIX)", ContentType::Numeric, 10)
                      .AddItem(YearName, "Year (Local)", ContentType::Numeric, 4)
                      .AddItem(MonthName, "Month (Local)", ContentType::Numeric, 2)
                      .AddItem(DayName, "Day (Local)", ContentType::Numeric, 2)
                      .AddItem(HourName, "Hour (Local)", ContentType::Numeric, 2)
                      .AddItem(MinuteName, "Minute (Local)", ContentType::Numeric, 2)
                      .AddItem(SecondName, "Second (Local)", ContentType::Numeric, 2)
                      .AddItem(RecordOccurrenceName, "Record Occurrence", ContentType::Numeric, 5)
                      .AddItem(ItemOccurrenceName, "Item Occurrence", ContentType::Numeric, 5)
                      .AddItem(SubitemOccurrenceName, "Subitem Occurrence", ContentType::Numeric, 5)
                      .AddItem(NoteName, "Note", ContentType::Alpha, 999);

    return dictionary_creator.ReleaseDictionary();
}


struct ExtractNotesTask::IdLink
{
    size_t level_number;
    const CaseItem* input_case_item;
    const CaseItem* notes_case_item;
};


void ExtractNotesTask::SetUpIdLinks()
{
    ASSERT(m_idLinks == nullptr && m_notesCaseAccess != nullptr);

    m_idLinks = std::make_unique<std::vector<IdLink>>();

    for( const CDictItem* const id_item : m_caseAccess->GetDataDict().GetIdItems() )
    {
        m_idLinks->emplace_back(IdLink
            {
                id_item->GetLevel()->GetLevelNumber(),
                m_caseAccess->LookupCaseItem(*id_item),
                m_notesCaseAccess->LookupCaseItem(ExtractNotesTask::NamePrefix + id_item->GetName())
            });
    }
}


struct ExtractNotesTask::NoteLinks
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


void ExtractNotesTask::SetUpNoteLinks()
{
    ASSERT(m_noteLinks == nullptr && m_notesCaseAccess != nullptr);

    m_noteLinks = std::make_unique<NoteLinks>(
        NoteLinks
        {
            m_notesCaseAccess->LookupCaseItem(FieldNameName),
            m_notesCaseAccess->LookupCaseItem(OperatorIdName),
            m_notesCaseAccess->LookupCaseItem(TimestampName),
            m_notesCaseAccess->LookupCaseItem(YearName),
            m_notesCaseAccess->LookupCaseItem(MonthName),
            m_notesCaseAccess->LookupCaseItem(DayName),
            m_notesCaseAccess->LookupCaseItem(HourName),
            m_notesCaseAccess->LookupCaseItem(MinuteName),
            m_notesCaseAccess->LookupCaseItem(SecondName),
            m_notesCaseAccess->LookupCaseItem(RecordOccurrenceName),
            m_notesCaseAccess->LookupCaseItem(ItemOccurrenceName),
            m_notesCaseAccess->LookupCaseItem(SubitemOccurrenceName),
            m_notesCaseAccess->LookupCaseItem(NoteName)
        });
}


const CaseLevel* ExtractNotesTask::FindCaseLevel(Case& data_case, const std::string& level_key) const
{
    const std::vector<CaseLevel*> input_case_levels = data_case.GetAllCaseLevels();

    const auto& case_level_search = std::find_if(input_case_levels.cbegin(), input_case_levels.cend(),
        [&](const CaseLevel* const case_level)
        {
            return ( UTF8_TODO::GetUtf8(case_level->GetLevelKey()) == level_key );
        });

    return ( case_level_search == input_case_levels.cend() ) ? nullptr :
                                                               *case_level_search;
}


void ExtractNotesTask::CopyNotesToCase(Case& data_case, const std::vector<const Note*>& notes)
{
    ASSERT(m_idLinks != nullptr && m_noteLinks != nullptr);
    ASSERT(!notes.empty());

    m_notesCase->Reset();

    // copy the root IDs
    CaseItemIndex input_id_index = data_case.GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();
    CaseItemIndex notes_id_index = m_notesCase->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();

    for( const IdLink& id_link : *m_idLinks )
    {
        if( id_link.level_number == 0 )
        {
            CaseItemHelpers::CopyValue(*id_link.input_case_item, input_id_index,
                                       *id_link.notes_case_item, notes_id_index);
        }
    }

    // copy the notes
    CaseRecord& notes_case_record = m_notesCase->GetRootCaseLevel().GetCaseRecord(0);
    const size_t note_occurrences = std::min(MaxNotesPerCase, notes.size());
    notes_case_record.SetNumberOccurrences(note_occurrences);
    CaseItemIndex notes_index = notes_case_record.GetCaseItemIndex();

    for( const Note* const note : notes )
    {
        // if on a level, set the IDs for that level
        if( !note->GetNamedReference().GetLevelKey().empty() )
        {
            const CaseLevel* const case_level = FindCaseLevel(data_case, note->GetNamedReference().GetLevelKey());

            if( case_level != nullptr )
            {
                // copy over the level keys
                for( size_t level_number = 1; level_number <= case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber(); ++level_number )
                {
                    const CaseLevel* this_case_level = case_level;

                    while( this_case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() != level_number )
                        this_case_level = &this_case_level->GetParentCaseLevel();

                    for( const IdLink& id_link : *m_idLinks )
                    {
                        if( id_link.level_number == level_number )
                        {
                            CaseItemHelpers::CopyValue(*id_link.input_case_item, this_case_level->GetIdCaseRecord().GetCaseItemIndex(),
                                                       *id_link.notes_case_item, notes_index);
                        }
                    }
                }
            }
        }

        CaseItemHelpers::SetValue(*m_noteLinks->field_name_case_item, notes_index, note->GetNamedReference().GetName());
        CaseItemHelpers::SetValue(*m_noteLinks->operator_id_case_item, notes_index, note->GetOperatorId());
        CaseItemHelpers::SetValue(*m_noteLinks->timestamp_case_item, notes_index, static_cast<double>(note->GetModifiedDateTime()));

        // convert the timestamp to local time
        const DateTime::Components date_time_components = DateTime::TimeToComponents(note->GetModifiedDateTime(), true);
        CaseItemHelpers::SetValue(*m_noteLinks->year_case_item, notes_index, date_time_components.year);
        CaseItemHelpers::SetValue(*m_noteLinks->month_case_item, notes_index, date_time_components.month);
        CaseItemHelpers::SetValue(*m_noteLinks->day_case_item, notes_index, date_time_components.day);
        CaseItemHelpers::SetValue(*m_noteLinks->hour_case_item, notes_index, date_time_components.hour);
        CaseItemHelpers::SetValue(*m_noteLinks->minute_case_item, notes_index, date_time_components.minute);
        CaseItemHelpers::SetValue(*m_noteLinks->second_case_item, notes_index, date_time_components.second);

        if( note->GetNamedReference().HasOccurrences() )
        {
            const std::vector<size_t> one_based_occurrences = note->GetNamedReference().GetOneBasedOccurrences();
            ASSERT(one_based_occurrences.size() == 3);

            if( one_based_occurrences[0] > 0 )
                CaseItemHelpers::SetValue(*m_noteLinks->record_occurrence_case_item, notes_index, static_cast<double>(one_based_occurrences[0]));

            if( one_based_occurrences[1] > 0 )
                CaseItemHelpers::SetValue(*m_noteLinks->item_occurrence_case_item, notes_index, static_cast<double>(one_based_occurrences[1]));

            if( one_based_occurrences[2] > 0 )
                CaseItemHelpers::SetValue(*m_noteLinks->subitem_occurrence_case_item, notes_index, static_cast<double>(one_based_occurrences[2]));
        }

        CaseItemHelpers::SetValue(*m_noteLinks->note_case_item, notes_index, note->GetContentSharableString());

        ++m_extractedNotes;

        notes_index.IncrementRecordOccurrence();

        if( notes_index.GetRecordOccurrence() == note_occurrences )
            break;
    }
}
