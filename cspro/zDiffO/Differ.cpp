#include "stdafx.h"
#include "Differ.h"
#include "DiffSpec.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilF/ProcessSummaryDlg.h>
#include <zAppO/PFF.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItemPrinter.h>
#include <zCaseO/StdioCaseConstructionReporter.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>


namespace
{
    constexpr size_t ProgressBarCaseUpdateFrequency = 100;
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
        throw CSProException("You must specify a listing filename.");

    m_log = std::make_unique<CStdioFileUnicode>();

    if( !m_log->Open(pff.GetListingFName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText) )
        throw CSProException(_T("There was an error creating the listing file:\n\n%s"), pff.GetListingFName().GetString());

    bool run_success = false;

    try
    {
        // check the data file parameters
        if( !pff.GetSingleInputDataConnectionString().IsDefined() )
            throw CSProException("You must specify an input filename.");

        if( !pff.GetReferenceDataConnectionString().IsDefined() )
            throw CSProException("You must specify a reference filename.");

        if( pff.GetSingleInputDataConnectionString().IsFilenamePresent() && pff.GetSingleInputDataConnectionString().Equals(pff.GetReferenceDataConnectionString()) )
            throw CSProException("You must specify input and reference data files that are different from each other.");

        // load the diff spec if necessary
        if( m_diffSpec == nullptr )
        {
            m_diffSpec = std::make_shared<DiffSpec>();
            m_diffSpec->Load(CS2WS(pff.GetAppFName()), silent, std::move(embedded_dictionary));
        }

        // run the comparison
        InitializeComparison();

        Run(pff.GetSingleInputDataConnectionString(), pff.GetReferenceDataConnectionString());

        run_success = true;
    }

    catch( const CSProException& exception )
    {
        m_log->WriteFormattedLine(_T("*** %s"), exception.GetErrorMessage().c_str());
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
    m_caseAccess = std::make_shared<CaseAccess>(m_diffSpec->GetDictionary());

    for( const auto& [item, occurrence] : m_diffSpec->GetDiffItems() )
        m_caseAccess->SetUseDictionaryItem(*item);

    m_caseAccess->Initialize();

    // group each item by record
    for( const auto& [item, occurrence] : m_diffSpec->GetDiffItems() )
    {
        if( m_recordsCaseItemsMap.find(item->GetRecord()) == m_recordsCaseItemsMap.cend() )
            m_recordsCaseItemsMap.try_emplace(item->GetRecord(), std::vector<std::tuple<const CaseItem*, size_t>>());

        m_recordsCaseItemsMap[item->GetRecord()].emplace_back(m_caseAccess->LookupCaseItem(*item), occurrence.value_or(0));
    }
}


void Differ::Run(const ConnectionString& input_connection_string, const ConnectionString& output_connection_string)
{
    // open the repositories
    std::unique_ptr<DataRepository> input_repository = DataRepository::CreateAndOpen(m_caseAccess,
        input_connection_string, DataRepositoryAccess::ReadOnly, DataRepositoryOpenFlag::OpenMustExist);

    std::unique_ptr<DataRepository> reference_repository = DataRepository::CreateAndOpen(m_caseAccess,
        output_connection_string, DataRepositoryAccess::ReadOnly, DataRepositoryOpenFlag::OpenMustExist);

    // write the listing header
    constexpr wstring_view Divider_sv = _T("-----------------------------------------------------------------------------------------------");

    m_log->WriteFormattedLine(_T("Input:      %s"), input_repository->GetName(DataRepositoryNameType::ForListing).GetString());
    m_log->WriteFormattedLine(_T("Reference:  %s"), reference_repository->GetName(DataRepositoryNameType::ForListing).GetString());
    m_log->WriteLine(Divider_sv);

    m_log->WriteLine(_T("Case Id"));
    m_log->WriteLine(_T("  Item                                                     Input               Reference"));
    m_log->WriteLine(Divider_sv);
    m_log->WriteLine();

    // setup the cases and the reporter
    std::shared_ptr<ProcessSummary> process_summary = m_diffSpec->GetDictionary().CreateProcessSummary();
    auto case_construction_reporter = std::make_shared<StdioCaseConstructionReporter>(*m_log, process_summary);

    auto input_case = m_caseAccess->CreateCase();
    input_case->SetCaseConstructionReporter(case_construction_reporter);

    auto reference_case = m_caseAccess->CreateCase();
    reference_case->SetCaseConstructionReporter(case_construction_reporter);


    // display a progress bar while doing the comparison, allocating 25% of the
    // comparison time to reading the keys
    ProcessSummaryDlg process_summary_dlg;

    process_summary_dlg.SetTask([&]
    {
        // setup the progress bar handling
        constexpr double CaseKeyReadingPercent = 25;
        double progress_bar_value = 0;
        double progress_bar_increment_value = 0;
        size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
        size_t progress_bar_counts = 0;

        auto check_and_update_progress_bar = [&](const auto& get_key, const size_t counts = 1)
        {
            if( process_summary_dlg.IsCanceled() )
                throw UserCanceledException();

            progress_bar_counts += counts;

            if( --progress_bar_update_counter == 0 )
            {
                progress_bar_value += progress_bar_increment_value * progress_bar_counts;
                process_summary->SetPercentSourceRead(progress_bar_value);
                process_summary_dlg.SetKey(get_key());
                progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
                progress_bar_counts = 0;
            }
        };


        // get a listing of all of the keys in the files
        process_summary_dlg.Initialize(_T("Reading keys..."), process_summary);
        process_summary_dlg.SetSource(FormatText(_T("Input / Reference Data: %s / %s"),
                                                 input_repository->GetName(DataRepositoryNameType::Concise).GetString(),
                                                 reference_repository->GetName(DataRepositoryNameType::Concise).GetString()));

        const size_t total_case_keys = input_repository->GetNumberCases() + reference_repository->GetNumberCases();
        progress_bar_increment_value = CaseKeyReadingPercent / std::max<size_t>(total_case_keys, 1);

        auto get_all_case_keys = [&](DataRepository& repository)
        {
            std::vector<std::wstring> keys;
            CaseKey case_key;

            std::unique_ptr<CaseIterator> case_key_iterator = repository.CreateCaseKeyIterator(
                ( m_diffSpec->GetDiffOrder() == DiffSpec::DiffOrder::Indexed ) ? CaseIterationMethod::KeyOrder : CaseIterationMethod::SequentialOrder,
                CaseIterationOrder::Ascending);

            while( case_key_iterator->NextCaseKey(case_key) )
            {
                keys.emplace_back(case_key.GetKey());

                check_and_update_progress_bar([&] { return case_key.GetKey(); });
            }

            return keys;
        };

        std::vector<std::wstring> input_keys = get_all_case_keys(*input_repository);
        std::vector<std::wstring> reference_keys = get_all_case_keys(*reference_repository);


        // compare the differences
        process_summary_dlg.Initialize(_T("Comparing..."), process_summary);

        progress_bar_value = CaseKeyReadingPercent;
        progress_bar_increment_value = ( 100 - CaseKeyReadingPercent ) / std::max<size_t>(total_case_keys, 1);

        while( !input_keys.empty() || !reference_keys.empty() )
        {
            auto input_index = input_keys.cbegin();
            auto reference_index = reference_keys.cbegin();

            if( !input_keys.empty() && !reference_keys.empty() )
            {
                if( m_diffSpec->GetDiffOrder() == DiffSpec::DiffOrder::Indexed )
                {
                    // if both have keys remaining, compare the current key for each
                    const int key_comparison = input_keys.front().compare(reference_keys.front());

                    if( key_comparison < 0 )
                    {
                        reference_index = reference_keys.cend();
                    }

                    else if( key_comparison > 0 )
                    {
                        input_index = input_keys.cend();
                    }
                }

                else // sequential order
                {
                    // search for the input key in the reference keys
                    reference_index = std::find(reference_keys.cbegin(), reference_keys.cend(), input_keys.front());
                }
            }

            // there are no more input keys or the reference key comes before the input key
            if( input_index == input_keys.cend() )
            {
                if( m_diffSpec->GetDiffMethod() == DiffSpec::DiffMethod::BothWays )
                {
                    const std::wstring key = FormatTextCS2WS(_T("[%s]"), NewlineSubstitutor::NewlineToUnicodeNL(*reference_index).c_str());
                    m_log->WriteFormattedLine(_T("%-59sCase Missing"), key.c_str());
                    m_log->WriteLine();
                    m_differencesExist = true;
                }

                check_and_update_progress_bar([&] { return WS2CS(*reference_index); });

                reference_keys.erase(reference_index, reference_index + 1);
            }

            // there are no more reference keys or the input key comes before the reference key
            else if( reference_index == reference_keys.cend() )
            {
                const std::wstring key = FormatTextCS2WS(_T("[%s]"), NewlineSubstitutor::NewlineToUnicodeNL(*input_index).c_str());
                m_log->WriteFormattedLine(_T("%-59s%-20sCase Missing"), key.c_str(), _T(""));
                m_log->WriteLine();
                m_differencesExist = true;

                check_and_update_progress_bar([&] { return WS2CS(*input_index); });

                input_keys.erase(input_index, input_index + 1);
            }

            // the keys are the same, so we must compare them
            else
            {
                input_repository->ReadCase(*input_case, WS2CS(*input_index));
                reference_repository->ReadCase(*reference_case, WS2CS(*reference_index));

                CompareCase(*input_case, *reference_case);

                check_and_update_progress_bar([&] { return WS2CS(*input_index); }, 2);

                input_keys.erase(input_index, input_index + 1);
                reference_keys.erase(reference_index, reference_index + 1);
            }
        }

        input_repository->Close();
        reference_repository->Close();

        if( !m_differencesExist )
            m_log->WriteLine(_T("No differences were found."));
    });

    process_summary_dlg.DoModal();

    process_summary_dlg.RethrowTaskExceptions();
}


void Differ::CompareCase(const Case& input_case, const Case& reference_case)
{
    // within a case, we will compare all of the levels in sorted key order
    std::vector<const CaseLevel*> input_case_levels = input_case.GetAllCaseLevels();
    std::vector<const CaseLevel*> reference_case_levels = reference_case.GetAllCaseLevels();

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
                const std::wstring key = FormatTextCS2WS(_T("[%s%s]"), NewlineSubstitutor::NewlineToUnicodeNL(reference_case.GetKey()).GetString(),
                                                                       NewlineSubstitutor::NewlineToUnicodeNL((*reference_index)->GetLevelKey()).GetString());
                m_log->WriteFormattedLine(_T("%-59sLevel Missing"), key.c_str());
                m_log->WriteLine();
                m_differencesExist = true;
            }

            reference_case_levels.erase(reference_index, reference_index + 1);
        }

        // there are no more reference levels or the input level comes before the reference level
        else if( reference_index == reference_case_levels.cend() )
        {
            const std::wstring key = FormatTextCS2WS(_T("[%s%s]"), NewlineSubstitutor::NewlineToUnicodeNL(input_case.GetKey()).GetString(),
                                                                   NewlineSubstitutor::NewlineToUnicodeNL((*input_index)->GetLevelKey()).GetString());
            m_log->WriteFormattedLine(_T("%-59s%-20sLevel Missing"), key.c_str(), _T(""));
            m_log->WriteLine();
            m_differencesExist = true;

            input_case_levels.erase(input_index, input_index + 1);
        }

        // the keys are the same, so we must compare the records in the level
        else
        {
            CompareLevel(*(*input_index), *(*reference_index));

            input_case_levels.erase(input_index, input_index + 1);
            reference_case_levels.erase(reference_index, reference_index + 1);
        }
    }
}


void Differ::CompareLevel(const CaseLevel& input_case_level, const CaseLevel& reference_case_level)
{
    std::wstring differences_text;

    for( size_t record_number = 0; record_number < input_case_level.GetNumberCaseRecords(); ++record_number )
    {
        const CaseRecord& input_case_record = input_case_level.GetCaseRecord(record_number);
        const CDictRecord& dictionary_record = input_case_record.GetCaseRecordMetadata().GetDictionaryRecord();

        // see if anything on the record has been marked for comparison
        const auto& case_items_for_record = m_recordsCaseItemsMap.find(&dictionary_record);

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
                const std::wstring record_label = FormatTextCS2WS(_T("  %s(%d)"),
                                                                  m_diffSpec->GetShowLabels() ? dictionary_record.GetLabel().GetString() : dictionary_record.GetName().GetString(),
                                                                  static_cast<int>(record_occurrence) + 1);
                SO::AppendFormat(differences_text, _T("%-59sRecord Missing\n"), record_label.c_str());
            }
        }

        for( size_t record_occurrence = reference_case_record.GetNumberOccurrences();
             record_occurrence < input_case_record.GetNumberOccurrences();
             ++record_occurrence )
        {
            const std::wstring record_label = FormatTextCS2WS(_T("  %s(%d)"),
                                                              m_diffSpec->GetShowLabels() ? dictionary_record.GetLabel().GetString() : dictionary_record.GetName().GetString(),
                                                              static_cast<int>(record_occurrence) + 1);
            SO::AppendFormat(differences_text, _T("%-59s%-20sRecord Missing\n"), record_label.c_str(), _T(""));
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
                    const std::wstring input_value = NewlineSubstitutor::NewlineToUnicodeNL(m_caseItemPrinter->GetText(*case_item, input_index));
                    const std::wstring reference_value = NewlineSubstitutor::NewlineToUnicodeNL(m_caseItemPrinter->GetText(*case_item, reference_index));

                    const std::wstring item_label = FormatTextCS2WS(_T("  %s%s"),
                                                                    m_diffSpec->GetShowLabels() ? case_item->GetDictionaryItem().GetLabel().Left(52).GetString() :
                                                                                                  case_item->GetDictionaryItem().GetName().Left(32).GetString(),
                                                                    input_index.GetMinimalOccurrencesText(*case_item).GetString());

                    if( input_value.length() < 20 && reference_value.length() < 20 )
                    {
                        SO::AppendFormat(differences_text, _T("%-59s%-20s%s\n"), item_label.c_str(), input_value.c_str(), reference_value.c_str());
                    }

                    else
                    {
                        // if the length of the item is long, output it on different lines
                        SO::AppendFormat(differences_text, _T("%-59sInp:%s\n"), item_label.c_str(), input_value.c_str());
                        SO::AppendFormat(differences_text, _T("%-59sRef:%s\n"), _T(""), reference_value.c_str());
                    }
                }
            }
        }
    }

    if( !differences_text.empty() )
    {
        const std::wstring key = FormatTextCS2WS(_T("[%s%s]"), NewlineSubstitutor::NewlineToUnicodeNL(input_case_level.GetCase().GetKey()).GetString(),
                                                               NewlineSubstitutor::NewlineToUnicodeNL(input_case_level.GetLevelKey()).GetString());
        m_log->WriteLine(key);
        m_log->WriteString(differences_text);
        m_log->WriteLine();
        m_differencesExist = true;
    }
}
