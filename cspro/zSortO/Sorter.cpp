#include "stdafx.h"
#include "Sorter.h"
#include "SortableKeyDatabase.h"
#include "SortSpec.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilF/ProcessSummaryDlg.h>
#include <zAppO/PFF.h>
#include <zCaseO/Case.h>
#include <zCaseO/StdioCaseConstructionReporter.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    constexpr size_t ProgressBarCaseUpdateFrequency = 100;
}


struct SortCaseItem
{
    const SortSpec::SortItem& sort_item;
    const CaseItem* case_item;
    size_t record_number;
};


Sorter::Sorter(std::shared_ptr<SortSpec> sort_spec/* = nullptr*/)
    :   m_sortSpec(std::move(sort_spec))
{
}


Sorter::~Sorter()
{
}


Sorter::RunSuccess Sorter::Run(const PFF& pff, const bool silent, std::shared_ptr<const CDataDict> embedded_dictionary/* = nullptr*/)
{
    //  open the log file
    if( pff.GetListingFName().IsEmpty() )
        throw CSProException("You must specify a listing filename.");

    m_log = std::make_unique<CStdioFileUnicode>();

    if( !m_log->Open(pff.GetListingFName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText) )
        throw CSProException(_T("There was an error creating the listing file:\n\n%s"), pff.GetListingFName().GetString());

    RunSuccess run_success = RunSuccess::Errors;

    try
    {
        // load the sort spec if necessary
        if( m_sortSpec == nullptr )
        {
            m_sortSpec = std::make_shared<SortSpec>();
            m_sortSpec->Load(CS2WS(pff.GetAppFName()), silent, embedded_dictionary);
        }

        if( m_sortSpec->GetSortItems().empty() )
            throw CSProException("No sort items were specified.");

        // run the sort
        run_success = Run(pff);
    }

    catch( const CSProException& exception )
    {
        if( dynamic_cast<const UserCanceledException*>(&exception) != nullptr )
            run_success = RunSuccess::UserCanceled;

        m_log->WriteFormattedString(_T("*** %s\n\n"), exception.GetErrorMessage().c_str());

        // on error, try to delete the sorted data
        try
        {
            if( m_sortedRepository != nullptr )
                m_sortedRepository->DeleteRepository();

            m_log->WriteString(_T("*** Fatal errors during sort: sorted data deleted!\n\n"));
        }

        catch( const DataRepositoryException::Error& )
        {
            m_log->WriteString(_T("*** Fatal errors during sort: sorted data could not be deleted but is invalid!\n\n"));
        }
    }

    m_inputRepository.reset();
    m_sortedRepository.reset();
    m_sortableKeyDatabase.reset();

    // close the log and potentially view the listing and results
    m_log->Close();

    if( pff.GetViewListing() == ALWAYS || ( pff.GetViewListing() == ONERROR && run_success != RunSuccess::Success ) )
        pff.ViewListing();

    if( pff.GetViewResultsFlag() && run_success == RunSuccess::Success )
        PFF::ViewResults(pff.GetSingleOutputDataConnectionString());

    return run_success;
}


Sorter::RunSuccess Sorter::Run(const PFF& pff)
{
    // check the data files
    if( !pff.GetSingleInputDataConnectionString().IsDefined() )
        throw CSProException("You must specify an input filename.");

    if( !pff.GetSingleOutputDataConnectionString().IsDefined() )
        throw CSProException("You must specify an output filename.");


    const CDataDict* dictionary_to_use_for_sorting = &m_sortSpec->GetDictionary();

    if( m_sortSpec->IsRecordSort() )
    {
        const DataRepositoryType data_repository_type = pff.GetSingleInputDataConnectionString().GetType();

        if( !DataRepositoryHelpers::TypeSupportsRecordSort(data_repository_type) )
            throw CSProException(_T("The input data source type '%s' does not support record sorts."), ToString(data_repository_type));

        CreateFlattenedRecordSortDictionary();
        dictionary_to_use_for_sorting = m_flattenedRecordSortDictionary.get();
    }


    // open the repositories
    InitializeCaseAccess(*dictionary_to_use_for_sorting);

    m_inputRepository = DataRepository::CreateAndOpen(
        m_sortSpec->IsRecordSort() ? m_caseAccess : m_firstPassCaseAccess,
        pff.GetSingleInputDataConnectionString(),
        m_sortSpec->IsRecordSort() ? DataRepositoryAccess::BatchInput : DataRepositoryAccess::ReadOnly,
        DataRepositoryOpenFlag::OpenMustExist);

    m_sortedRepository = DataRepository::CreateAndOpen(
        m_caseAccess,
        pff.GetSingleOutputDataConnectionString(),
        DataRepositoryAccess::BatchOutput,
        DataRepositoryOpenFlag::CreateNew);

    m_log->WriteFormattedString(_T("Input Data:   %s\n"), m_inputRepository->GetName(DataRepositoryNameType::ForListing).GetString());
    m_log->WriteFormattedString(_T("Sorted Data:  %s\n\n"), m_sortedRepository->GetName(DataRepositoryNameType::ForListing).GetString());


    // set up the sortable key database
    m_sortableKeyDatabase = std::make_unique<SortableKeyDatabase>(m_sortSpec->IsRecordSort() ? SortableKeyDatabase::SortType::RecordSort :
                                                                                               SortableKeyDatabase::SortType::CaseSort);

    for( const SortSpec::SortItem& sort_item : m_sortSpec->GetSortItems() )
        m_sortableKeyDatabase->AddKeyType(sort_item.dict_item->GetContentType(), ( sort_item.order == SortSpec::SortOrder::Ascending ));

    if( !m_sortableKeyDatabase->Open() )
        throw CSProException("There was a problem opening the sortable key database.");


    // run the sort
    return ( m_sortSpec->IsCaseSort() ) ? RunCaseSort() :
                                          RunRecordSort();
}


void Sorter::InitializeCaseAccess(const CDataDict& dictionary)
{
    m_caseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(dictionary);
    m_caseAccess->SetCaseConstructionReporter(std::make_unique<StdioCaseConstructionReporter>(*m_log));

    // if not doing a record sort, we can also create a minimal first pass case access
    if( m_sortSpec->IsCaseSort() )
    {
        m_firstPassCaseAccess = std::make_unique<CaseAccess>(dictionary);

        for( const SortSpec::SortItem& sort_item : m_sortSpec->GetSortItems() )
            m_firstPassCaseAccess->SetUseDictionaryItem(*sort_item.dict_item);

        m_firstPassCaseAccess->Initialize();
    }

    CaseAccess& case_access_to_use_for_access = m_sortSpec->IsRecordSort() ? *m_caseAccess :
                                                                             *m_firstPassCaseAccess;

    for( const SortSpec::SortItem& sort_item : m_sortSpec->GetSortItems() )
    {
        SortCaseItem& sort_case_item = *m_sortCaseItems.emplace_back(std::make_unique<SortCaseItem>(SortCaseItem
            {
                sort_item,
                nullptr,
                SIZE_MAX
            }));

        if( !m_sortSpec->IsRecordTypeItem(sort_item) )
        {
            sort_case_item.case_item = case_access_to_use_for_access.LookupCaseItem(sort_item.dict_item->GetName());
            sort_case_item.record_number = sort_case_item.case_item->GetDictionaryItem().GetRecord()->GetRecordNumber();
        }
    }
}


Sorter::RunSuccess Sorter::RunCaseSort()
{
    ProcessSummaryDlg process_summary_dlg;
    RunSuccess run_success = RunSuccess::Success;


    process_summary_dlg.SetTask([&]
    {
        std::shared_ptr<ProcessSummary> process_summary;

        // setup the progress bar handling
        double progress_bar_value = 0;
        double progress_bar_increment_value = 0;
        size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;

        auto check_and_update_progress_bar = [&](const auto& get_key)
        {
            if( process_summary_dlg.IsCanceled() )
                throw UserCanceledException();

            if( --progress_bar_update_counter == 0 )
            {
                progress_bar_value += progress_bar_increment_value;
                process_summary->SetPercentSourceRead(progress_bar_value);
                process_summary_dlg.SetKey(get_key());
                progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
            }
        };

        const size_t number_cases = m_inputRepository->GetNumberCases();

        if( number_cases > 0 )
            progress_bar_increment_value = std::min(100.0, 100 / ( static_cast<double>(number_cases) / ProgressBarCaseUpdateFrequency ));

        // the progress bar increment value is divided by two because there are two passes
        progress_bar_increment_value /= 2;


        // read the cases
        process_summary = m_firstPassCaseAccess->GetDataDict().CreateProcessSummary();
        process_summary_dlg.Initialize(_T("Sorting..."), process_summary);
        process_summary_dlg.SetSource(FormatText(_T("Input Data: %s"), m_inputRepository->GetName(DataRepositoryNameType::Full).GetString()));

        std::unique_ptr<Case> first_pass_case = m_firstPassCaseAccess->CreateCase();
        auto first_pass_case_construction_reporter = std::make_shared<StdioCaseConstructionReporter>(*m_log, process_summary);
        first_pass_case->SetCaseConstructionReporter(first_pass_case_construction_reporter);

        std::unique_ptr<CaseIterator> case_iterator = m_inputRepository->CreateCaseIterator(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Ascending);

        while( case_iterator->NextCase(*first_pass_case) )
        {
            m_sortableKeyDatabase->InitCaseInfo(first_pass_case->GetPositionInRepository());

            // generate the sortable key
            CaseLevel& root_case_level = first_pass_case->GetRootCaseLevel();

            for( const SortCaseItem& sort_case_item : VI_V(m_sortCaseItems) )
            {
                CaseRecord& case_record = root_case_level.GetCaseRecord(sort_case_item.record_number);

                // if using something on a record that doesn't exist, create it to get its default value
                if( !case_record.HasOccurrences() )
                    case_record.SetNumberOccurrences(1);

                const CaseItemIndex index = case_record.GetCaseItemIndex();
                m_sortableKeyDatabase->AddCaseKeyValue(*sort_case_item.case_item, index);
            }

            m_sortableKeyDatabase->AddCase();

            check_and_update_progress_bar([&] { return first_pass_case->GetKey(); });
        }


        // once the file has been read, start using the full case access
        m_inputRepository->ModifyCaseAccess(m_caseAccess);

        process_summary = m_caseAccess->GetDataDict().CreateProcessSummary();
        process_summary->SetPercentSourceRead(progress_bar_value);

        process_summary_dlg.Initialize(_T("Writing..."), process_summary);
        process_summary_dlg.SetSource(FormatText(_T("Sorted Data: %s"), m_sortedRepository->GetName(DataRepositoryNameType::Concise).GetString()));


        std::unique_ptr<Case> data_case = m_caseAccess->CreateCase();

        // don't log case construction errors on the second pass
        auto second_pass_case_construction_reporter = std::make_shared<CaseConstructionReporter>(process_summary);
        data_case->SetCaseConstructionReporter(second_pass_case_construction_reporter);


        // write the sorted cases
        double position_in_repository;

        while( m_sortableKeyDatabase->NextPosition(&position_in_repository) )
        {
            m_inputRepository->ReadCase(*data_case, position_in_repository);
            m_sortedRepository->WriteCase(*data_case);

            check_and_update_progress_bar([&] { return data_case->GetKey(); });
        }


        // write the summary information
        m_log->WriteString(_T("Summary\n"));
        m_log->WriteFormattedString(_T("    Questionnaires: %d\n    Records: %d\n"),
                                    static_cast<int>(second_pass_case_construction_reporter->GetCaseLevelCount(0)),
                                    static_cast<int>(second_pass_case_construction_reporter->GetRecordCount() - second_pass_case_construction_reporter->GetErasedRecordCount()));

        if( first_pass_case_construction_reporter->HadErrors() || second_pass_case_construction_reporter->GetBadRecordCount() != 0 )
            run_success = RunSuccess::Errors;
    });

    process_summary_dlg.DoModal();

    process_summary_dlg.RethrowTaskExceptions();

    return run_success;
}


void Sorter::CreateFlattenedRecordSortDictionary()
{
    // flatten all levels and make all records not required to
    // create a suitable dictionary for record sorting
    std::shared_ptr<const CDataDict> base_dictionary = m_sortSpec->GetSharedDictionary();

    m_flattenedRecordSortDictionary = std::make_unique<CDataDict>(*base_dictionary);
    m_requiredFlattenedDictRecords.clear();

    DictLevel& output_dict_level = m_flattenedRecordSortDictionary->GetLevel(0);

    // remove all extra levels and add each record on those levels to the first level
    for( size_t level_number = 1; level_number < base_dictionary->GetNumLevels(); ++level_number )
    {
        m_flattenedRecordSortDictionary->RemoveLevel(1);

        const DictLevel& input_dict_level = base_dictionary->GetLevel(level_number);

        for( int item_index = 0; item_index < input_dict_level.GetIdItemsRec()->GetNumItems(); ++item_index )
            output_dict_level.GetIdItemsRec()->AddItem(input_dict_level.GetIdItemsRec()->GetItem(item_index));

        for( int record_index = 0; record_index < input_dict_level.GetNumRecords(); ++record_index )
            output_dict_level.AddRecord(input_dict_level.GetRecord(record_index));
    }

    // make all records not required
    for( int record_index = 0; record_index < output_dict_level.GetNumRecords(); ++record_index )
    {
        CDictRecord* output_dict_record = output_dict_level.GetRecord(record_index);

        if( output_dict_record->GetRequired() )
        {
            m_requiredFlattenedDictRecords.insert(output_dict_record);
            output_dict_record->SetRequired(false);
        }
    }

    m_flattenedRecordSortDictionary->UpdatePointers();
}


Sorter::RunSuccess Sorter::RunRecordSort()
{
    ProcessSummaryDlg process_summary_dlg;
    RunSuccess run_success = RunSuccess::Success;


    process_summary_dlg.SetTask([&]
    {
        std::shared_ptr<ProcessSummary> process_summary;

        // setup the progress bar handling
        size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;

        auto check_and_update_progress_bar = [&](const auto& get_key, const auto& get_percent)
        {
            if( process_summary_dlg.IsCanceled() )
                throw UserCanceledException();

            if( --progress_bar_update_counter == 0 )
            {
                process_summary->SetPercentSourceRead(get_percent());
                process_summary_dlg.SetKey(get_key());
                progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
            }
        };


        // read the cases
        process_summary = m_caseAccess->GetDataDict().CreateProcessSummary();
        process_summary_dlg.Initialize(_T("Sorting..."), process_summary);
        process_summary_dlg.SetSource(FormatText(_T("Input Data: %s"), m_inputRepository->GetName(DataRepositoryNameType::Concise).GetString()));

        size_t records_processed = 0;

        std::unique_ptr<Case> data_case = m_caseAccess->CreateCase();
        auto first_pass_case_construction_reporter = std::make_shared<StdioCaseConstructionReporter>(*m_log, process_summary);
        data_case->SetCaseConstructionReporter(first_pass_case_construction_reporter);

        std::unique_ptr<CaseIterator> case_iterator = m_inputRepository->CreateCaseIterator(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Ascending);

        while( case_iterator->NextCase(*data_case) )
        {
            // process the records on each level (which have been flattened to a single level)
            ASSERT(data_case->GetAllCaseLevels().size() == 1);
            CaseLevel& root_case_level = data_case->GetRootCaseLevel();

            // make sure that all records that are used for the sort exist (to ensure that at least default values exist)
            std::set<const CaseRecord*> added_case_records;

            for( const SortCaseItem& sort_case_item : VI_V(m_sortCaseItems) )
            {
                if( sort_case_item.case_item != nullptr )
                {
                    CaseRecord& case_record = root_case_level.GetCaseRecord(sort_case_item.record_number);

                    if( !case_record.HasOccurrences() )
                    {
                        case_record.SetNumberOccurrences(1);
                        added_case_records.insert(&case_record);
                    }
                }
            }

            // get the binary representation of the ID record
            std::vector<std::byte> id_binary_buffer = root_case_level.GetIdCaseRecord().GetBinaryValues(0);

            for( size_t record_number = 0; record_number < root_case_level.GetNumberCaseRecords(); ++record_number )
            {
                const CaseRecord& case_record_for_record_sort = root_case_level.GetCaseRecord(record_number);

                // don't add records that were added above (but weren't in the data file)
                if( added_case_records.find(&case_record_for_record_sort) != added_case_records.end() )
                    continue;

                for( size_t record_occurrence = 0; record_occurrence < case_record_for_record_sort.GetNumberOccurrences(); ++record_occurrence )
                {
                    ++records_processed;

                    // add the binary representation of the record
                    const std::vector<std::byte> record_binary_buffer = case_record_for_record_sort.GetBinaryValues(record_occurrence);

                    m_sortableKeyDatabase->InitRecordInfo(case_record_for_record_sort.GetCaseRecordMetadata().GetRecordIndex(),
                                                          id_binary_buffer.data(), id_binary_buffer.size(),
                                                          record_binary_buffer.data(), record_binary_buffer.size());

                    // generate the sortable key
                    for( const SortCaseItem& sort_case_item : VI_V(m_sortCaseItems) )
                    {
                        // case_item not being set to anything means that we need to add the record type
                        if( sort_case_item.case_item == nullptr )
                        {
                            ASSERT(sort_case_item.sort_item.dict_item->GetContentType() == ContentType::Alpha);

                            m_sortableKeyDatabase->AddCaseKeyValue(case_record_for_record_sort.GetCaseRecordMetadata().GetDictionaryRecord().GetRecTypeVal());
                        }

                        else
                        {
                            const CaseRecord& case_record_for_sortable_key = root_case_level.GetCaseRecord(sort_case_item.record_number);
                            ASSERT(case_record_for_sortable_key.HasOccurrences());

                            const size_t record_occurrence_for_sortable_key = ( &case_record_for_record_sort == &case_record_for_sortable_key ) ? record_occurrence :
                                                                                                                                                  0;

                            const CaseItemIndex index = case_record_for_sortable_key.GetCaseItemIndex(record_occurrence_for_sortable_key);
                            m_sortableKeyDatabase->AddCaseKeyValue(*sort_case_item.case_item, index);
                        }
                    }

                    m_sortableKeyDatabase->AddCase();
                }
            }

            // update the progress bar (divided by two because the record sort has two passes)
            check_and_update_progress_bar([&]{ return data_case->GetKey(); },
                                          [&]{ return ( case_iterator->GetPercentRead() / 2 ); });
        }


        // write out the sorted file
        double progress_bar_value = 50;
        const double progress_bar_increment_value = 50 / std::max<double>(records_processed, 1.0);

        process_summary = m_caseAccess->GetDataDict().CreateProcessSummary();
        process_summary->SetPercentSourceRead(progress_bar_value);

        process_summary_dlg.Initialize(_T("Writing..."), process_summary);
        process_summary_dlg.SetSource(FormatText(_T("Sorted Data: %s"), m_sortedRepository->GetName(DataRepositoryNameType::Concise).GetString()));


        auto second_pass_case_construction_reporter = std::make_shared<StdioCaseConstructionReporter>(*m_log, process_summary);
        data_case->SetCaseConstructionReporter(second_pass_case_construction_reporter);


        // this will be used to determine whether or not there are duplicate keys (rather than use any indexer code)
        SortableKeyDatabase duplicate_key_database(SortableKeyDatabase::SortType::CaseOnly);

        if( !duplicate_key_database.Open() )
            throw CSProException("There was a problem opening the sortable key database.");


        // a routine for writing out cases
        CaseLevel& root_case_level = data_case->GetRootCaseLevel();
        bool at_least_one_record_processed = false;

        auto write_case = [&]()
        {
            if( at_least_one_record_processed )
            {
                m_sortedRepository->WriteCase(*data_case);
                process_summary->IncrementCaseLevelsRead(0);

                // warn where there are missing required records
                for( size_t record_number = 0; record_number < root_case_level.GetNumberCaseRecords(); ++record_number )
                {
                    const CaseRecord& case_record = root_case_level.GetCaseRecord(record_number);
                    const CDictRecord& dict_record = case_record.GetCaseRecordMetadata().GetDictionaryRecord();

                    if( !case_record.HasOccurrences() && m_requiredFlattenedDictRecords.find(&dict_record) != m_requiredFlattenedDictRecords.end() )
                    {
                        m_log->WriteFormattedString(_T("*** [%s]\n"), NewlineSubstitutor::NewlineToUnicodeNL(data_case->GetKey()).GetString());
                        m_log->WriteString(_T("*** This record type is required in every questionnaire. It was not found in this questionnaire.\n"));
                        m_log->WriteFormattedString(_T("        Record type: %s\n\n"), dict_record.GetLabel().GetString());
                        run_success = RunSuccess::SuccessWithStructuralErrors;
                    }
                }

                // warn when the sorted file will have duplicates
                if( duplicate_key_database.CaseExists(data_case->GetKey()) )
                {
                    m_log->WriteFormattedString(_T("*** [%s]\n"), NewlineSubstitutor::NewlineToUnicodeNL(data_case->GetKey()).GetString());
                    m_log->WriteString(_T("*** A case with this ID has already been output. You will have to remove duplicates before safely using this file.\n\n"));
                }

                else
                {
                    duplicate_key_database.InitCaseInfo(0, data_case->GetKey());
                    duplicate_key_database.AddCase();
                }
            }
        };

        // process each record, constructing them into cases
        std::vector<std::byte> previous_id_binary_buffer;
        std::vector<std::byte> id_binary_buffer;
        std::vector<std::byte> record_binary_buffer;
        size_t record_index;

        while( m_sortableKeyDatabase->NextRecord(&record_index, &id_binary_buffer, &record_binary_buffer) )
        {
            process_summary->IncrementAttributesRead();

            // if the ID has changed, write out the last case
            if( previous_id_binary_buffer != id_binary_buffer )
            {
                write_case();
                at_least_one_record_processed = true;

                // start a new case and add the IDs
                data_case->Reset();

                root_case_level.GetIdCaseRecord().SetBinaryValues(0, id_binary_buffer.data());
                previous_id_binary_buffer = id_binary_buffer;
            }

            // add the record
            CaseRecord& case_record = root_case_level.GetCaseRecord(record_index);
            const CDictRecord& dict_record = case_record.GetCaseRecordMetadata().GetDictionaryRecord();

            // warn if too many records
            const size_t record_occurrence = case_record.GetNumberOccurrences();

            if( record_occurrence == dict_record.GetMaxRecs() )
            {
                second_pass_case_construction_reporter->TooManyRecordOccurrences(*data_case, dict_record.GetName(), dict_record.GetMaxRecs());
                continue;
            }

            // otherwise add the record
            case_record.SetNumberOccurrences(record_occurrence + 1);
            case_record.SetBinaryValues(record_occurrence, record_binary_buffer.data());


            // update the progress bar
            progress_bar_value += progress_bar_increment_value;

            check_and_update_progress_bar([&]{ return data_case->GetKey(); },
                                          [&]{ return progress_bar_value; });
        }

        // write the last case
        write_case();


        // write the summary information
        m_log->WriteString(_T("Summary\n"));
        m_log->WriteFormattedString(_T("    Questionnaires: %d\n    Records: %d\n"),
                                    static_cast<int>(second_pass_case_construction_reporter->GetCaseLevelCount(0)),
                                    static_cast<int>(first_pass_case_construction_reporter->GetRecordCount() - first_pass_case_construction_reporter->GetErasedRecordCount()));

        if( first_pass_case_construction_reporter->HadErrors() )
            run_success = RunSuccess::Errors;
    });

    process_summary_dlg.DoModal();

    process_summary_dlg.RethrowTaskExceptions();

    return run_success;
}
