#include "stdafx.h"
#include "Differ.h"
#include "DiffSpec.h"
#include <zToolsO/File.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilF/ProcessSummaryDlg.h>
#include <zAppO/PFF.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItemPrinter.h>
#include <zCaseO/StdioCaseConstructionReporter.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    constexpr size_t ProgressBarCaseUpdateFrequency = 100;
    constexpr double CaseKeyReadingPercent          = 25;
}


Differ::Differ(std::shared_ptr<DiffSpec> diff_spec/* = nullptr*/)
    :   m_diffSpec(std::move(diff_spec)),
        m_differencesExist(false)
{
}


Differ::~Differ()
{
}


bool Differ::Run(const PFF& pff, const bool silent, std::shared_ptr<const CDataDict> embedded_dictionary/* = nullptr*/)
{
    //  open the log file
    if( pff.GetListingFName().IsEmpty() )
        throw CSProException("You must specify a listing file.");

    m_log = std::make_unique<FileIO::TextFile>();
    m_log->OpenForTextWritingCreate(pff.GetListingFName());

    bool run_success = false;

    try
    {
        // check the data file parameters
        if( !pff.GetSingleInputDataConnectionString().IsDefined() )
            throw CSProException("You must specify an input data source.");

        if( !pff.GetReferenceDataConnectionString().IsDefined() )
            throw CSProException("You must specify a reference data source.");

        if( pff.GetSingleInputDataConnectionString().SharesResource(pff.GetReferenceDataConnectionString()) )
            throw CSProException("You must specify input and reference data sources that are different from each other.");

        // load the diff spec if necessary
        if( m_diffSpec == nullptr )
        {
            m_diffSpec = std::make_unique<DiffSpec>();
            m_diffSpec->Load(UTF8_TODO::GetUtf8(pff.GetAppFName()), silent, std::move(embedded_dictionary));
        }

        // run the comparison
        InitializeComparison();

        Run(pff.GetSingleInputDataConnectionString(), pff.GetReferenceDataConnectionString());

        run_success = true;
    }

    catch( const CSProException& exception )
    {
        m_log->WriteFormattedLine("*** %s", exception.what());
    }

    // close the log and potentially view the listing
    m_log->Close();

    if( run_success && !m_differencesExist )
    {
        HandleNoDifferences(pff);
    }

    else if( pff.GetViewListing() == ALWAYS || ( pff.GetViewListing() == ONERROR && ( !run_success || m_differencesExist ) ) )
    {
        pff.ViewListing();
    }

    return run_success;
}


void Differ::HandleNoDifferences(const PFF& pff)
{
    if( pff.GetViewListing() == ALWAYS )
        pff.ViewListing();
}


void Differ::InitializeComparison()
{
    m_caseItemPrinter = std::make_unique<CaseItemPrinter>(CaseItemPrinter::Format::Code);

    // setup the case access
    m_caseAccess = std::make_unique<CaseAccess>(m_diffSpec->GetDictionary());

    for( const auto& [item, occurrence] : m_diffSpec->GetDiffItems() )
        m_caseAccess->SetUseDictionaryItem(*item);

    m_caseAccess->Initialize();

    // group each item by record
    for( const auto& [dict_item, occurrence] : m_diffSpec->GetDiffItems() )
    {
        if( m_recordsCaseItemsMap.find(dict_item->GetRecord()) == m_recordsCaseItemsMap.cend() )
            m_recordsCaseItemsMap.try_emplace(dict_item->GetRecord(), std::vector<std::tuple<const CaseItem*, size_t>>());

        m_recordsCaseItemsMap[dict_item->GetRecord()].emplace_back(m_caseAccess->LookupCaseItem(*dict_item), occurrence.value_or(0));
    }
}


struct Differ::RunData
{
    std::unique_ptr<DataRepository> input_repository;
    std::unique_ptr<DataRepository> reference_repository;

    std::shared_ptr<StdioCaseConstructionReporter> case_construction_reporter;
    std::unique_ptr<Case> input_case;
    std::unique_ptr<Case> reference_case;

    std::shared_ptr<ProcessSummary> process_summary;
    ProcessSummaryDlg process_summary_dlg;

    double progress_bar_value = 0;
    double progress_bar_increment_value = 0;
    size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
    size_t progress_bar_counts = 0;
};


struct Differ::CaseCompareData
{
    const Case& input_case;
    const Case& reference_case;
    std::optional<std::tuple<const std::string*, const std::string*>> duplicate_case_uuids;
};


void Differ::Run(const ConnectionString& input_connection_string, const ConnectionString& output_connection_string)
{
    RunData rd;

    // open the repositories
    rd.input_repository = DataRepository::CreateAndOpen(
        m_caseAccess,
        input_connection_string,
        DataRepositoryAccess::ReadOnly,
        DataRepositoryOpenFlag::OpenMustExist
    );

    rd.reference_repository = DataRepository::CreateAndOpen(
        m_caseAccess,
        output_connection_string,
        DataRepositoryAccess::ReadOnly,
        DataRepositoryOpenFlag::OpenMustExist
    );

    // write the listing header
    constexpr std::string_view Divider_sv = "-----------------------------------------------------------------------------------------------";

    m_log->WriteLine("Input:      " + rd.input_repository->GetName(DataRepositoryNameType::ForListing));
    m_log->WriteLine("Reference:  " + rd.reference_repository->GetName(DataRepositoryNameType::ForListing));
    m_log->WriteLine(Divider_sv);

    m_log->WriteLine("Case Key");
    m_log->WriteLine("  Item                                                     Input               Reference");
    m_log->WriteLine(Divider_sv);
    m_log->WriteLine();

    // set up the cases and the reporter
    rd.input_case = m_caseAccess->CreateCase();
    rd.reference_case = m_caseAccess->CreateCase();

    rd.case_construction_reporter = std::make_unique<StdioCaseConstructionReporter>(*m_log, rd.process_summary);
    rd.input_case->SetCaseConstructionReporter(rd.case_construction_reporter);
    rd.reference_case->SetCaseConstructionReporter(rd.case_construction_reporter);

    rd.process_summary = m_diffSpec->GetDictionary().CreateProcessSummary();

    // a progress bar will be shown while RunCompare runs in a background thread
    rd.process_summary_dlg.SetTask([&]()
    {
        // when the data can contain duplicates, try to match cases based on UUID in addition to the key
        if( m_diffSpec->GetDiffOrder() == DiffSpec::DiffOrder::Indexed &&
            DataRepositoryHelpers::TypeUsesUuid(rd.input_repository->GetRepositoryType()) &&
            DataRepositoryHelpers::TypeUsesUuid(rd.reference_repository->GetRepositoryType()) )
        {
            RunCompare<true>(rd);
        }

        else
        {
            RunCompare<false>(rd);
        }
    });

    rd.process_summary_dlg.DoModal();

    rd.process_summary_dlg.RethrowTaskExceptions();
}


template<bool UseUuidMatching>
void Differ::RunCompare(RunData& rd)
{
    // get a listing of all of the keys in the files
    rd.process_summary_dlg.Initialize("Reading keys...", rd.process_summary);

    rd.process_summary_dlg.SetSource(
        FormatText("Input / Reference Data: %s / %s",
            rd.input_repository->GetName(DataRepositoryNameType::Concise).c_str(),
            rd.reference_repository->GetName(DataRepositoryNameType::Concise).c_str()
        )
    );

    const size_t total_case_keys = rd.input_repository->GetNumberCases() + rd.reference_repository->GetNumberCases();
    rd.progress_bar_increment_value = CaseKeyReadingPercent / std::max<size_t>(total_case_keys, 1);

    const bool indexed_order = ( m_diffSpec->GetDiffOrder() == DiffSpec::DiffOrder::Indexed );
    const CaseIterationMethod iteration_method = indexed_order ? CaseIterationMethod::KeyOrder : CaseIterationMethod::SequentialOrder;

    std::vector<MatchIdentifier<UseUuidMatching>> input_identifiers = GetIdentifiers<UseUuidMatching>(rd, *rd.input_repository, iteration_method);
    std::vector<MatchIdentifier<UseUuidMatching>> reference_identifiers = GetIdentifiers<UseUuidMatching>(rd, *rd.reference_repository, iteration_method);

    // compare the differences
    rd.process_summary_dlg.Initialize("Comparing...", rd.process_summary);

    rd.progress_bar_value = CaseKeyReadingPercent;
    rd.progress_bar_increment_value = ( 100 - CaseKeyReadingPercent ) / std::max<size_t>(total_case_keys, 1);

    std::optional<std::string> last_case_key_with_duplicates;

    CaseCompareData ccd
    {
        *rd.input_case,
        *rd.reference_case,
    };

    while( !input_identifiers.empty() || !reference_identifiers.empty() )
    {
        auto input_index = input_identifiers.cbegin();
        auto reference_index = reference_identifiers.cbegin();
        bool this_case_key_had_duplicates = false;

        if( !input_identifiers.empty() && !reference_identifiers.empty() )
        {
            const std::string& input_case_key = input_identifiers.front().GetKey();

            // indexed order
            if( indexed_order )
            {
                // if both have keys remaining, compare the current key for each
                const int key_comparison = input_case_key.compare(reference_identifiers.front().GetKey());

                if( key_comparison < 0 )
                {
                    reference_index = reference_identifiers.cend();
                }

                else if( key_comparison > 0 )
                {
                    input_index = input_identifiers.cend();
                }

                // when matching using UUIDs, we potentially need to match a different case when there are duplicates
                else if constexpr(UseUuidMatching)
                {
                    if( IdentifiersContainDuplicates(input_identifiers, reference_identifiers) )
                    {
                        last_case_key_with_duplicates = input_case_key;
                        this_case_key_had_duplicates = true;
                        reference_index = MatchCaseByUuid(rd, input_identifiers, reference_identifiers);
                    }

                    else if( input_case_key == last_case_key_with_duplicates )
                    {
                        this_case_key_had_duplicates = true;
                    }
                }
            }

            // sequential order
            else if constexpr(!UseUuidMatching)
            {
                // search for the first reference key that matches the input key
                reference_index = std::find_if(reference_identifiers.cbegin(), reference_identifiers.cend(),
                    [&](const auto& reference_identifier)
                    {
                        return ( input_case_key == reference_identifier.GetKey() );
                    });
            }

            else
            {
                ASSERT(false);
            }
        }

        // there are no more input keys or the reference key comes before the input key
        if( input_index == input_identifiers.cend() )
        {
            if( m_diffSpec->GetDiffMethod() == DiffSpec::DiffMethod::BothWays )
            {
                const std::string key = FormatText("[%s]", NewlineSubstitutor::NewlineToUnicodeNL(reference_index->GetKey()).c_str());
                m_log->WriteFormattedLine("%-59sCase Missing", key.c_str());

                if constexpr(UseUuidMatching)
                {
                    this_case_key_had_duplicates = ( reference_index->GetKey() == last_case_key_with_duplicates );

                    if( this_case_key_had_duplicates )
                        m_log->WriteFormattedLine("[UUID: %s]", reference_index->uuid.c_str());
                }

                m_log->WriteLine();
                m_differencesExist = true;
            }

            CheckAndUpdateProcessBar(rd, reference_index->GetKey(), 1);

            reference_identifiers.erase(reference_index, reference_index + 1);
        }

        // there are no more reference keys or the input key comes before the reference key
        else if( reference_index == reference_identifiers.cend() )
        {
            const std::string key = FormatText("[%s]", NewlineSubstitutor::NewlineToUnicodeNL(input_index->GetKey()).c_str());
            m_log->WriteFormattedLine("%-59s%-20sCase Missing", key.c_str(), "");

            if constexpr(UseUuidMatching)
            {
                this_case_key_had_duplicates = ( input_index->GetKey() == last_case_key_with_duplicates );

                if( this_case_key_had_duplicates )
                    m_log->WriteFormattedLine("[UUID: %s]", input_index->uuid.c_str());
            }

            m_log->WriteLine();
            m_differencesExist = true;

            CheckAndUpdateProcessBar(rd, input_index->GetKey(), 1);

            input_identifiers.erase(input_index, input_index + 1);
        }

        // the keys are the same, so we must compare them
        else
        {
            if constexpr(UseUuidMatching)
            {
                if( this_case_key_had_duplicates )
                {
                    ccd.duplicate_case_uuids.emplace(&input_index->uuid, &reference_index->uuid);
                }

                else
                {
                    ccd.duplicate_case_uuids.reset();
                }
            }

            ASSERT(this_case_key_had_duplicates == ccd.duplicate_case_uuids.has_value());

            // read the case by position (so that duplicate cases can be properly read)
            rd.input_repository->ReadCase(*rd.input_case, input_index->GetPositionInRepository());
            rd.reference_repository->ReadCase(*rd.reference_case, reference_index->GetPositionInRepository());

            CompareCase(ccd);

            CheckAndUpdateProcessBar(rd, input_index->GetKey(), 2);

            input_identifiers.erase(input_index, input_index + 1);
            reference_identifiers.erase(reference_index, reference_index + 1);
        }
    }

    rd.input_repository->Close();
    rd.reference_repository->Close();

    if( !m_differencesExist )
        m_log->WriteLine("No differences were found.");
}


void Differ::CheckAndUpdateProcessBar(RunData& rd, const std::string& key, const size_t counts)
{
    if( rd.process_summary_dlg.IsCanceled() )
        throw UserCanceledException();

    rd.progress_bar_counts += counts;

    if( --rd.progress_bar_update_counter == 0 )
    {
        rd.progress_bar_value += rd.progress_bar_increment_value * rd.progress_bar_counts;
        rd.process_summary->SetPercentSourceRead(rd.progress_bar_value);
        rd.process_summary_dlg.SetKey(key);
        rd.progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
        rd.progress_bar_counts = 0;
    }
}


template<bool UseUuidMatching>
std::vector<Differ::MatchIdentifier<UseUuidMatching>> Differ::GetIdentifiers(
    RunData& rd,
    DataRepository& repository,
    const CaseIterationMethod iteration_method) const
{
    std::vector<MatchIdentifier<UseUuidMatching>> identifiers;
    CaseKey case_key;

    const std::unique_ptr<CaseIterator> case_key_iterator = repository.CreateCaseKeyIterator(
        iteration_method,
        CaseIterationOrder::Ascending
    );

    while( case_key_iterator->NextCaseKey(case_key) )
    {
        identifiers.emplace_back(case_key);
        CheckAndUpdateProcessBar(rd, case_key.GetKey(), 1);
    }

    return identifiers;
}


bool Differ::IdentifiersContainDuplicates(const std::vector<MatchIdentifier<true>>& input_identifiers,
                                          const std::vector<MatchIdentifier<true>>& reference_identifiers)
{
    ASSERT(!input_identifiers.empty() && !reference_identifiers.empty());

    const std::string& case_key = input_identifiers.front().GetKey();
    ASSERT(case_key == reference_identifiers.front().GetKey());

    return ( ( input_identifiers.size() > 1 && case_key == input_identifiers[1].GetKey() ) ||
             ( reference_identifiers.size() > 1 && case_key == reference_identifiers[1].GetKey() ) );
}


std::vector<Differ::MatchIdentifier<true>>::const_iterator Differ::MatchCaseByUuid(
    RunData& rd,
    std::vector<MatchIdentifier<true>>& input_identifiers,
    std::vector<MatchIdentifier<true>>& reference_identifiers)
{
    ASSERT(IdentifiersContainDuplicates(input_identifiers, reference_identifiers));

    const std::string& case_key = input_identifiers.front().GetKey();

    // load the UUIDs for all of the cases with this key (if they have not already been loaded)
    auto load_uuids = [&](std::vector<MatchIdentifier<true>>& identifiers, DataRepository& repository, Case& data_case)
    {
        auto identifiers_itr = identifiers.begin();

        do
        {
            ASSERT(case_key == identifiers_itr->GetKey());

            if( identifiers_itr->uuid.empty() )
            {
                repository.ReadCase(data_case, identifiers_itr->GetPositionInRepository());
                identifiers_itr->uuid = data_case.GetUuid();
            }

            ASSERT(!identifiers_itr->uuid.empty());

        } while( ++identifiers_itr != identifiers.end() && case_key == identifiers_itr->GetKey() );

        return identifiers_itr;
    };

    const auto input_with_key_end = load_uuids(input_identifiers, *rd.input_repository, *rd.input_case);
    const auto reference_with_key_end = load_uuids(reference_identifiers, *rd.reference_repository, *rd.reference_case);

    const std::string& uuid_to_match = input_identifiers.front().uuid;

    // search for a matching UUID, which is a perfect match
    auto reference_lookup = std::find_if(reference_identifiers.begin(), reference_with_key_end,
        [&](const auto& reference_identifier)
        {
            return ( uuid_to_match == reference_identifier.uuid );
        });

    if( reference_lookup != reference_with_key_end )
        return reference_lookup;

    // otherwise search for a reference case that has a UUID that is not matched in a subsequent input case
    auto reference_itr = reference_identifiers.cbegin();

    do
    {
        ASSERT(case_key == reference_itr->GetKey());

        auto input_lookup = std::find_if(input_identifiers.begin() + 1, input_with_key_end,
            [&](const auto& input_identifier)
            {
                ASSERT(input_identifier.GetKey() == reference_itr->GetKey());
                return ( input_identifier.uuid == reference_itr->uuid );
            });

        if( input_lookup == input_with_key_end )
            return reference_itr;

    } while( ++reference_itr != reference_identifiers.cend() && case_key == reference_itr->GetKey() );

    // no matching case
    return reference_identifiers.cend();
}


void Differ::CompareCase(const CaseCompareData& ccd)
{
    // within a case, we will compare all of the levels in sorted key order
    std::vector<const CaseLevel*> input_case_levels = ccd.input_case.GetAllCaseLevels();
    std::vector<const CaseLevel*> reference_case_levels = ccd.reference_case.GetAllCaseLevels();

    auto sort_case_levels = [](std::vector<const CaseLevel*>& case_levels)
    {
        std::sort(case_levels.begin(), case_levels.end(),
            [](const CaseLevel* case_level1, const CaseLevel* case_level2)
            {
                return ( case_level1->GetLevelKey().Compare(case_level2->GetLevelKey()) < 0 );
            });
    };

    sort_case_levels(input_case_levels);
    sort_case_levels(reference_case_levels);

    auto write_case_uuids_for_duplicates = [&]()
    {
        if( !ccd.duplicate_case_uuids.has_value() )
            return;

        m_log->WriteFormattedLine("[UUID: %s]", std::get<0>(*ccd.duplicate_case_uuids)->c_str());

        if( *std::get<0>(*ccd.duplicate_case_uuids) != *std::get<1>(*ccd.duplicate_case_uuids) )
            m_log->WriteFormattedLine("[      %s]", std::get<1>(*ccd.duplicate_case_uuids)->c_str());
    };

    while( !input_case_levels.empty() || !reference_case_levels.empty() )
    {
        auto input_index = input_case_levels.cbegin();
        auto reference_index = reference_case_levels.cbegin();

        if( !input_case_levels.empty() && !reference_case_levels.empty() )
        {
            // if both have keys remaining, compare the current key for each
            const int key_comparison = input_case_levels.front()->GetLevelKey().Compare(reference_case_levels.front()->GetLevelKey());

            if( key_comparison < 0 )
            {
                reference_index = reference_case_levels.cend();
            }

            else if( key_comparison > 0 )
            {
                input_index = input_case_levels.cend();
            }
        }

        // there are no more input levels or the reference level comes before the input level
        if( input_index == input_case_levels.cend() )
        {
            if( m_diffSpec->GetDiffMethod() == DiffSpec::DiffMethod::BothWays )
            {
                const std::string key = FormatText("[%s%s]", NewlineSubstitutor::NewlineToUnicodeNL(ccd.reference_case.GetKey()).c_str(),
                                                             NewlineSubstitutor::NewlineToUnicodeNL(UTF8_TODO::GetUtf8((*reference_index)->GetLevelKey())).c_str());
                m_log->WriteFormattedLine("%-59sLevel Missing", key.c_str());
                write_case_uuids_for_duplicates();
                m_log->WriteLine();
                m_differencesExist = true;
            }

            reference_case_levels.erase(reference_index, reference_index + 1);
        }

        // there are no more reference levels or the input level comes before the reference level
        else if( reference_index == reference_case_levels.cend() )
        {
            const std::string key = FormatText("[%s%s]", NewlineSubstitutor::NewlineToUnicodeNL(ccd.input_case.GetKey()).c_str(),
                                                         NewlineSubstitutor::NewlineToUnicodeNL(UTF8_TODO::GetUtf8((*input_index)->GetLevelKey())).c_str());
            m_log->WriteFormattedLine("%-59s%-20sLevel Missing", key.c_str(), "");
            write_case_uuids_for_duplicates();
            m_log->WriteLine();
            m_differencesExist = true;

            input_case_levels.erase(input_index, input_index + 1);
        }

        // the keys are the same, so we must compare the records in the level
        else
        {
            const CaseLevel& input_case_level = *(*input_index);
            const CaseLevel& reference_case_level = *(*reference_index);
            const std::string differences_text = CompareLevel(input_case_level, reference_case_level);

            if( !differences_text.empty() )
            {
                const std::string key = FormatText("[%s%s]", NewlineSubstitutor::NewlineToUnicodeNL(ccd.input_case.GetKey()).c_str(),
                                                             NewlineSubstitutor::NewlineToUnicodeNL(UTF8_TODO::GetUtf8(input_case_level.GetLevelKey())).c_str());
                m_log->WriteLine(key);
                write_case_uuids_for_duplicates();
                m_log->WriteString(differences_text);
                m_log->WriteLine();
                m_differencesExist = true;
            }

            input_case_levels.erase(input_index, input_index + 1);
            reference_case_levels.erase(reference_index, reference_index + 1);
        }
    }
}


std::string Differ::CompareLevel(const CaseLevel& input_case_level, const CaseLevel& reference_case_level)
{
    std::string differences_text;

    for( size_t record_number = 0; record_number < input_case_level.GetNumberCaseRecords(); ++record_number )
    {
        const CaseRecord& input_case_record = input_case_level.GetCaseRecord(record_number);
        const CDictRecord& dict_record = input_case_record.GetCaseRecordMetadata().GetDictRecord();

        // see if anything on the record has been marked for comparison
        const auto& case_items_for_record = m_recordsCaseItemsMap.find(&dict_record);

        if( case_items_for_record == m_recordsCaseItemsMap.cend() )
            continue;

        const CaseRecord& reference_case_record = reference_case_level.GetCaseRecord(record_number);

        // display information about missing records
        if( m_diffSpec->GetDiffMethod() == DiffSpec::DiffMethod::BothWays )
        {
            for( size_t record_occurrence = input_case_record.GetNumberOccurrences();
                 record_occurrence < reference_case_record.GetNumberOccurrences();
                 ++record_occurrence )
            {
                const std::string record_label = FormatText("  %s(%d)",
                                                            m_diffSpec->GetShowLabels() ? UTF8_TODO::GetUtf8(dict_record.GetLabel()).c_str() :
                                                                                          dict_record.GetName().c_str(),
                                                            static_cast<int>(record_occurrence) + 1);
                differences_text.append(FormatText("%-59sRecord Missing\n", record_label.c_str()));
            }
        }

        for( size_t record_occurrence = reference_case_record.GetNumberOccurrences();
             record_occurrence < input_case_record.GetNumberOccurrences();
             ++record_occurrence )
        {
            const std::string record_label = FormatText("  %s(%d)",
                                                        m_diffSpec->GetShowLabels() ? UTF8_TODO::GetUtf8(dict_record.GetLabel()).c_str() :
                                                                                      dict_record.GetName().c_str(),
                                                        static_cast<int>(record_occurrence) + 1);

            differences_text.append(FormatText("%-59s%-20sRecord Missing\n", record_label.c_str(), ""));
        }

        const size_t shared_record_occurrences = std::min(input_case_record.GetNumberOccurrences(), reference_case_record.GetNumberOccurrences());

        for( size_t record_occurrence = 0; record_occurrence < shared_record_occurrences; ++record_occurrence )
        {
            CaseItemIndex input_index = input_case_record.GetCaseItemIndex(record_occurrence);
            CaseItemIndex reference_index = reference_case_record.GetCaseItemIndex(record_occurrence);

            // process the items for comparison
            for( const auto& [case_item, occurrence] : case_items_for_record->second )
            {
                // set the item/subitem indices
                input_index.SetItemSubitemOccurrence(*case_item, occurrence);
                reference_index.SetItemSubitemOccurrence(*case_item, occurrence);

                // compare the values
                if( case_item->CompareValues(input_index, reference_index) != 0 )
                {
                    const std::string input_value = NewlineSubstitutor::NewlineToUnicodeNL(m_caseItemPrinter->GetText(*case_item, input_index));
                    const std::string reference_value = NewlineSubstitutor::NewlineToUnicodeNL(m_caseItemPrinter->GetText(*case_item, reference_index));

                    const std::string item_label = FormatText("  %s%s",
                                                              m_diffSpec->GetShowLabels() ? UTF8_TODO::GetUtf8(case_item->GetDictItem().GetLabel().Left(52)).c_str() :
                                                                                            UTF8_TODO::GetUtf8(UTF8_TODO::GetCString(case_item->GetDictItem().GetName()).Left(32)).c_str(),
                                                              input_index.GetMinimalOccurrencesText(*case_item).c_str());

                    if( input_value.length() < 20 && reference_value.length() < 20 )
                    {
                        differences_text.append(FormatText("%-59s%-20s%s\n", item_label.c_str(), input_value.c_str(), reference_value.c_str()));
                    }

                    else
                    {
                        // if the length of the item is long, output it on different lines
                        differences_text.append(FormatText("%-59sInp:%s\n", item_label.c_str(), input_value.c_str()));
                        differences_text.append(FormatText("%-59sRef:%s\n", "", reference_value.c_str()));
                    }
                }
            }
        }
    }

    return differences_text;
}
