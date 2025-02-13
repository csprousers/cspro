#include "stdafx.h"
#include "Indexer.h"
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteHelpers.h>
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilO/BasicLogger.h>
#include <zUtilO/FileExtensions.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilF/ProcessSummaryDlg.h>
#include <zAppO/PFF.h>
#include <zCaseO/Case.h>
#include <zCaseO/StdioCaseConstructionReporter.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zListingO/ListingHelpers.h>


namespace
{
    constexpr const char* CreateTableSql =
        "CREATE TABLE `Indexer` (`Key` TEXT NOT NULL, `FileIndex` INTEGER NOT NULL, "
        "`CaseIndex` INTEGER NOT NULL, `Position` REAL NOT NULL, `Bytes` INTEGER NOT NULL, `Line` INTEGER NOT NULL);";

    constexpr const char* PutCaseSql =
        "INSERT INTO `Indexer` (`Key`, `FileIndex`, `CaseIndex`, `Position`, `Bytes`, `Line`) VALUES( ?, ?, ?, ?, ?, ? );";

    constexpr const char* DeleteCasesByFileIndexSql =
        "DELETE FROM `Indexer` WHERE `FileIndex` = ?;";

    constexpr const char* CreateDuplicatesTableSql =
        "CREATE TABLE `Duplicates` AS "
        "SELECT `Key`, `FileIndex`, `CaseIndex`, `Position`, `Bytes`, `Line`, `DuplicateCount`, 1 AS `Keep` "
        "FROM `Indexer` "
        "JOIN ( SELECT `Key` AS `DuplicateKey`, COUNT(*) AS `DuplicateCount` FROM `Indexer` GROUP BY `Key` HAVING `DuplicateCount` > 1 ) ON `Key` = `DuplicateKey`;";

    constexpr const char* CreateDuplicatesTableIndexSql =
        "CREATE INDEX `DuplicatesIndex` ON `Duplicates` ( `Key`, `FileIndex` );";

    constexpr const char* DuplicateMinimalIteratorByKeySql =
        "SELECT `Key`, `FileIndex` FROM `Duplicates` "
        "ORDER BY `Key`, `FileIndex`;";

    constexpr const char* DuplicateFullIteratorByKeySql =
        "SELECT `Key`, `FileIndex`, `CaseIndex`, `Line`, `DuplicateCount` FROM `Duplicates` "
        "ORDER BY `Key`, `FileIndex`, `CaseIndex`;";

    constexpr const char* ReadDuplicatesIteratorSql =
        "SELECT `FileIndex`, `CaseIndex`, `Position`, `Bytes`, `Line` "
        "FROM `Duplicates` ORDER BY `FileIndex`;";

    constexpr const char* UpdateCaseDoNotKeepByIndexSql =
        "UPDATE `Duplicates` SET `Keep` = 0 WHERE `FileIndex` = ? AND `CaseIndex` = ?;";

    constexpr const char* UpdateCaseDoNotKeepByKeySql =
        "UPDATE `Duplicates` SET `Keep` = 0 WHERE `Key` = ?;";

    constexpr const char* WriteCaseIteratorSql =
        "SELECT `CaseIndex`, `Keep` FROM `Duplicates` "
        "WHERE `FileIndex` = ? AND `CaseIndex` > ? ORDER BY `CaseIndex` LIMIT 1;";

    CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(IndexerDatabaseException, "There was an error with the indexing database")

    constexpr size_t ProgressBarCaseUpdateFrequency = 100;
}


Indexer::Indexer()
    :   m_pff(nullptr),
        m_db(nullptr),
        m_stmtPutCase(nullptr),
        m_stmtDeleteCasesByFileIndex(nullptr),
        m_stmtMinimalDuplicateIteratorByKey(nullptr),
        m_stmtFullDuplicateIteratorByKey(nullptr),
        m_stmtReadDuplicatesIterator(nullptr),
        m_stmtUpdateCaseDoNotKeepByIndex(nullptr),
        m_stmtUpdateCaseDoNotKeepByKey(nullptr),
        m_stmtWriteCaseIterator(nullptr),
        m_currentlyProcessingIndexResult(nullptr),
        m_maxRepositoryNameLength(0),
        m_numberDuplicates(0),
        m_writeToCombinedFile(true)
{
}


Indexer::~Indexer()
{
    if( m_db == nullptr )
        return;

    safe_sqlite3_finalize(m_stmtWriteCaseIterator);
    safe_sqlite3_finalize(m_stmtUpdateCaseDoNotKeepByKey);
    safe_sqlite3_finalize(m_stmtUpdateCaseDoNotKeepByIndex);
    safe_sqlite3_finalize(m_stmtReadDuplicatesIterator);
    safe_sqlite3_finalize(m_stmtFullDuplicateIteratorByKey);
    safe_sqlite3_finalize(m_stmtMinimalDuplicateIteratorByKey);
    safe_sqlite3_finalize(m_stmtDeleteCasesByFileIndex);
    safe_sqlite3_finalize(m_stmtPutCase);
    sqlite3_close(m_db);
}


void Indexer::Run(const PFF& pff, const bool silent, std::shared_ptr<const CDataDict> embedded_dictionary/* = nullptr*/)
{
    m_pff = &pff;
    m_dictionary = std::move(embedded_dictionary);

    if( !SupportsInteractiveMode() )
    {
        switch( m_pff->GetDuplicateCase() )
        {
            case DuplicateCase::View:
            case DuplicateCase::Prompt:
            case DuplicateCase::PromptIfDifferent:
                throw CSProException("You can only run the indexer in an interactive mode when run directly from the Index Data tool.");
        }
    }


    StartLog();

    try
    {
        RunIndexer(silent);
    }

    catch( const CSProException& exception )
    {
        StopLog(&exception);

        // don't rethrow canceled exceptions
        if( !RethrowTerminatingException() || dynamic_cast<const UserCanceledException*>(&exception) != nullptr )
            return;

        throw;
    }

    StopLog();
}


void Indexer::RunIndexer(bool silent)
{
    // check that the PFF specifies all required properties
    if( m_pff->GetInputDataConnectionStrings().empty() )
        throw CSProException("You must specify at least one input file.");

    if( ( m_pff->GetDuplicateCase() != DuplicateCase::List && m_pff->GetDuplicateCase() != DuplicateCase::View ) &&
        !m_pff->GetSingleOutputDataConnectionString().IsDefined() )
    {
        throw CSProException("You must specify an output file.");
    }


    // open the dictionary
    if( m_dictionary == nullptr )
    {
        if( m_pff->GetInputDictFName().IsEmpty() )
            throw CSProException("You must specify a dictionary.");

        m_dictionary = CDataDict::InstantiateAndOpen(m_pff->GetInputDictFName(), silent);
    }


    // write the log header
    if( m_log != nullptr )
    {
        m_log->WriteFormattedLine(_T("%-15s %s"), _T("Dictionary:"), m_pff->GetInputDictFName().GetString());
        m_log->WriteLine();

        m_log->WriteFormattedLine(_T("%-15s %8s"), _T("Date"), Listing::GetSystemDate().c_str());

        const std::wstring time = Listing::GetSystemTime();
        m_log->WriteFormattedLine(_T("%-15s %s"), _T("Start Time"), time.c_str());

        m_log->WriteFormattedString(_T("%-15s %s"), _T("End Time"), time.c_str());
        m_endTimePosition = m_log->FlushAndGetPosition() - time.length();
        m_log->WriteLine();

        m_log->WriteLine();
    }


    // setup the case access
    m_keyReaderCaseAccess = std::make_shared<CaseAccess>(*m_dictionary);
    m_keyReaderCaseAccess->Initialize();

    m_fullCaseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(*m_dictionary);
    m_fullCaseAccess->SetCaseConstructionReporter(std::make_shared<StdioCaseConstructionReporter>(*m_log));

    // open the key information (in memory) database and create some prepared statements
    if( ( sqlite3_open("", &m_db) != SQLITE_OK ) ||
        ( sqlite3_exec(m_db, CreateTableSql, nullptr, nullptr, nullptr) != SQLITE_OK ) ||
        ( sqlite3_prepare_v2(m_db, PutCaseSql, -1, &m_stmtPutCase, nullptr ) != SQLITE_OK ) ||
        ( sqlite3_prepare_v2(m_db, DeleteCasesByFileIndexSql, -1, &m_stmtDeleteCasesByFileIndex, nullptr ) != SQLITE_OK ) )
    {
        throw IndexerDatabaseException();
    }


    // try to index the files
    size_t file_index = 0;

    for( const ConnectionString& connection_string : m_pff->GetInputDataConnectionStrings() )
    {
        m_currentlyProcessingIndexResult = &m_indexResults.emplace_back(file_index++, connection_string);
        IndexFile();
    }


    // calculate spacing details for reports and then write out any information about errors during the read
    bool files_without_errors_exist = false;
    BasicLogger error_log;

    CalculateOutputConnectionStrings();

    for( const IndexResult& index_result : m_indexResults )
    {
        m_maxRepositoryNameLength = std::max(m_maxRepositoryNameLength, index_result.repository_name.length());

        if( index_result.output_connection_string.IsFilenamePresent() )
            m_maxRepositoryNameLength = std::max(m_maxRepositoryNameLength, index_result.output_connection_string.GetFilename().length());
    }

    for( const IndexResult& index_result : m_indexResults )
    {
        if( index_result.exception_message.empty() )
        {
            files_without_errors_exist = true;
        }

        else
        {
            if( error_log.IsEmpty() )
                error_log.AppendLine(_T("The following data files could not be processed due to errors:"));

            error_log.AppendFormatLine(_T("    %-*s    [%s]"), static_cast<int>(m_maxRepositoryNameLength), index_result.repository_name.c_str(),
                                                               index_result.exception_message.c_str());
        }
    }

    if( !error_log.IsEmpty() )
    {
        if( m_log != nullptr )
            m_log->WriteLine(error_log.ToString());

        // if there are errors in an interactive mode, show the errors
        if( m_pff->GetDuplicateCase() != DuplicateCase::List && m_pff->GetDuplicateCase() != DuplicateCase::KeepFirst )
            DisplayInteractiveModeMessage(error_log.ToString());

        if( !files_without_errors_exist )
            throw CSProException("No data files could be processed.");
    }


    // calculate the number of duplicates for each file and then write out that summary information
    CalculateDuplicateCounts();

    // write out the detailed duplicate information
    WriteDuplicatesToLog();


    // if only creating a listing, we are finished
    if( m_pff->GetDuplicateCase() == DuplicateCase::List )
    {
        return;
    }

    // similarly, we are finished if viewing duplicates with none found
    else if( m_pff->GetDuplicateCase() == DuplicateCase::View && m_numberDuplicates == 0 )
    {
        DisplayInteractiveModeMessage(_T("No duplicate cases were found in your data files."));
        return;
    }

    // otherwise the duplicates have to be processed
    else if( m_numberDuplicates > 0 && m_pff->GetDuplicateCase() != DuplicateCase::KeepFirst )
    {
        ReadDuplicates();
        ChooseDuplicatesToKeep();
    }


    // write the cases with duplicates removed
    if( m_pff->GetDuplicateCase() != DuplicateCase::View )
        WriteCases();
}


void Indexer::StartLog()
{
    // view mode does not use a log
    if( m_pff->GetDuplicateCase() == DuplicateCase::View )
        return;

    //  open the log file
    if( m_pff->GetListingFName().IsEmpty() )
        throw CSProException("You must specify a listing filename.");

    m_log = std::make_unique<CStdioFileUnicode>();

    if( !m_log->Open(m_pff->GetListingFName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText) )
        throw CSProException(_T("There was an error creating the listing file:\n\n%s"), m_pff->GetListingFName().GetString());

    m_log->WriteLine(_T("CSIndex"));
    m_log->WriteLine(_T("-------"));
}


void Indexer::StopLog(const CSProException* exception/* = nullptr*/)
{
    if( m_log == nullptr )
        return;

    if( exception == nullptr )
    {
        m_log->WriteLine(_T("CSIndex completed successfully."));
    }

    else
    {
        m_log->WriteLine(_T("CSIndex terminated before completing:"));
        m_log->WriteLine();
        m_log->WriteLine(exception->GetErrorMessage());
    }

    // write the end time
    if( m_endTimePosition.has_value() )
    {
        m_log->Seek(*m_endTimePosition, SEEK_SET);
        m_log->WriteString(Listing::GetSystemTime().c_str());
        m_log->SeekToEnd();
    }

    m_log->Close();

    if( m_pff->GetViewListing() == ALWAYS || ( m_pff->GetViewListing() == ONERROR && exception != nullptr ) )
        m_pff->ViewListing();
}


void Indexer::IndexFile()
{
    try
    {
        std::unique_ptr<DataRepository> repository = DataRepository::Create(m_keyReaderCaseAccess,
            m_currentlyProcessingIndexResult->input_connection_string, DataRepositoryAccess::ReadOnly);

        // for text-based repositories, use the indexer callback
        if( DataRepositoryHelpers::DoesTypeUseIndexableText(repository->GetRepositoryType()) )
        {
            try
            {
                assert_cast<IndexableTextRepository&>(*repository).SetIndexerCallback(*this);
                repository->Open(m_currentlyProcessingIndexResult->input_connection_string, DataRepositoryOpenFlag::OpenMustExist);
                m_currentlyProcessingIndexResult->indexable_text_repository_index_created = true;
            }

            catch( const DataRepositoryException::DuplicateCaseWhileCreatingIndex& ) { }
        }

        // for other repository types, process each case key
        else
        {
            repository->Open(m_currentlyProcessingIndexResult->input_connection_string, DataRepositoryOpenFlag::OpenMustExist);

            // show a progress bar while reading cases
            ProcessSummaryDlg process_summary_dlg;

            process_summary_dlg.SetTask([&]
            {
                std::shared_ptr<ProcessSummary> process_summary = m_dictionary->CreateProcessSummary();
                process_summary_dlg.Initialize(_T("Reading keys..."), process_summary);
                process_summary_dlg.SetSource(FormatText(_T("Input Data: %s"), repository->GetName(DataRepositoryNameType::Full).GetString()));

                size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;

                CaseKey case_key;
                auto case_key_iterator = repository->CreateCaseKeyIterator(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Ascending);

                while( case_key_iterator->NextCaseKey(case_key) )
                {
                    IndexCallback(CS2WS(case_key.GetKey()), case_key.GetPositionInRepository(), 0, 0);

                    if( process_summary_dlg.IsCanceled() )
                        throw UserCanceledException();

                    process_summary->IncrementCaseLevelsRead(0);
                    process_summary->IncrementAttributesRead();

                    if( --progress_bar_update_counter == 0 )
                    {
                        process_summary->SetPercentSourceRead(case_key_iterator->GetPercentRead());
                        process_summary_dlg.SetKey(case_key.GetKey());
                        progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
                    }
                }
            });

            process_summary_dlg.DoModal();

            process_summary_dlg.RethrowTaskExceptions();
        }

        m_currentlyProcessingIndexResult->repository_name = repository->GetName(DataRepositoryNameType::Full);

        repository->Close();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        m_currentlyProcessingIndexResult->exception_message = exception.GetErrorMessage();

        // delete any case data for this file
        sqlite3_reset(m_stmtDeleteCasesByFileIndex);
        sqlite3_bind_int(m_stmtDeleteCasesByFileIndex, 1, static_cast<int>(m_currentlyProcessingIndexResult->file_index));

        if( sqlite3_step(m_stmtDeleteCasesByFileIndex) != SQLITE_DONE )
            throw IndexerDatabaseException();
    }
}


void Indexer::IndexCallback(const IndexableTextRepositoryIndexDetails& index_details)
{
    IndexCallback(index_details.key, static_cast<double>(index_details.position), index_details.bytes, index_details.line_number.value_or(0));
}


void Indexer::IndexCallback(const std::wstring& key, const double position_in_repository, const size_t bytes_for_case, const int64_t line_number)
{
    ++m_currentlyProcessingIndexResult->number_cases;

    sqlite3_reset(m_stmtPutCase);
    sqlite3_bind_text(m_stmtPutCase, 1, ToUtf8(key), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(m_stmtPutCase, 2, static_cast<int>(m_currentlyProcessingIndexResult->file_index));
    sqlite3_bind_int(m_stmtPutCase, 3, static_cast<int>(m_currentlyProcessingIndexResult->number_cases));
    sqlite3_bind_double(m_stmtPutCase, 4, position_in_repository);
    sqlite3_bind_int(m_stmtPutCase, 5, static_cast<int>(bytes_for_case));
    sqlite3_bind_int64(m_stmtPutCase, 6, line_number);

    if( sqlite3_step(m_stmtPutCase) != SQLITE_DONE )
        throw IndexerDatabaseException();
}


void Indexer::CalculateDuplicateCounts()
{
    // create a new table with just the duplicates
    if( sqlite3_exec(m_db, CreateDuplicatesTableSql, nullptr, nullptr, nullptr) != SQLITE_OK ||
        sqlite3_exec(m_db, CreateDuplicatesTableIndexSql, nullptr, nullptr, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, DuplicateMinimalIteratorByKeySql, -1, &m_stmtMinimalDuplicateIteratorByKey, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, DuplicateFullIteratorByKeySql, -1, &m_stmtFullDuplicateIteratorByKey, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, ReadDuplicatesIteratorSql, -1, &m_stmtReadDuplicatesIterator, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, UpdateCaseDoNotKeepByIndexSql, -1, &m_stmtUpdateCaseDoNotKeepByIndex, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, UpdateCaseDoNotKeepByKeySql, -1, &m_stmtUpdateCaseDoNotKeepByKey, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(m_db, WriteCaseIteratorSql, -1, &m_stmtWriteCaseIterator, nullptr) != SQLITE_OK )
    {
        throw IndexerDatabaseException();
    }

    std::string previous_duplicate_key;
    std::vector<size_t> file_indices;

    auto update_index_results = [&]()
    {
        if( file_indices.empty() )
            return;

        const bool duplicate_is_global = ( file_indices.front() != file_indices.back() );

        for( size_t i = 0; i < file_indices.size(); ++i )
        {
            const size_t file_index = file_indices[i];
            size_t number_duplicates_in_this_file = 1;

            while( ( i + 1 ) < file_indices.size() && file_index == file_indices[i + 1] )
            {
                ++number_duplicates_in_this_file;
                ++i;
            }

            if( duplicate_is_global )
                m_indexResults[file_index].number_global_duplicates += number_duplicates_in_this_file;

            if( number_duplicates_in_this_file > 1 )
                m_indexResults[file_index].number_internal_duplicates += number_duplicates_in_this_file;

            m_numberDuplicates += number_duplicates_in_this_file;
        }
    };

    sqlite3_reset(m_stmtMinimalDuplicateIteratorByKey);

    while( sqlite3_step(m_stmtMinimalDuplicateIteratorByKey) == SQLITE_ROW )
    {
        std::string key(reinterpret_cast<const char*>(sqlite3_column_text(m_stmtMinimalDuplicateIteratorByKey, 0)));
        const size_t file_index = sqlite3_column_int(m_stmtMinimalDuplicateIteratorByKey, 1);

        if( previous_duplicate_key.compare(key) != 0 )
        {
            // on a key change, process the previous duplicate
            update_index_results();

            previous_duplicate_key = std::move(key);
            file_indices.clear();
        }

        file_indices.emplace_back(file_index);
    }

    // process the last duplicate
    update_index_results();


    // write out the duplicate summary in three passes, the first for files without internal duplicates,
    // then for those with internal duplicates, and then for those with global duplicates
    if( m_log == nullptr )
        return;

    for( size_t pass = 0; pass < 3; ++pass )
    {
        bool header_written = false;

        for( const IndexResult& index_result : m_indexResults )
        {
            // skip files with errors
            if( !index_result.exception_message.empty() )
                continue;

            if( ( pass == 0 && index_result.number_internal_duplicates == 0 ) ||
                ( pass == 1 && index_result.number_internal_duplicates > 0 ) ||
                ( pass == 2 && index_result.number_global_duplicates > 0 ) )
            {
                if( !header_written )
                {
                    m_log->WriteLine(( pass == 0 ) ? _T("The following data files did not have internal duplicates:") :
                                     ( pass == 1 ) ? _T("The following data files had internal duplicates:") :
                                                     _T("The following data files had duplicates across files:"));
                    header_written = true;
                }

                std::wstring extra_text = FormatTextCS2WS(_T("%d case%s"),
                                                          static_cast<int>(index_result.number_cases), PluralizeWord(index_result.number_cases));

                if( index_result.number_internal_duplicates > 0 )
                {
                    SO::AppendFormat(extra_text, _T(", %d internal duplicate case%s"),
                                                 static_cast<int>(index_result.number_internal_duplicates), PluralizeWord(index_result.number_internal_duplicates));
                }

                if( pass == 2 )
                {
                    SO::AppendFormat(extra_text, _T(", %d duplicate case%s across files"),
                                                 static_cast<int>(index_result.number_global_duplicates), PluralizeWord(index_result.number_global_duplicates));
                }

                if( pass <= 1 && index_result.indexable_text_repository_index_created )
                {
                    const std::wstring index_filename = index_result.input_connection_string.GetFilename() + FileExtensions::Data::WithDot::IndexableTextIndex;
                    ASSERT(PortableFunctions::FileIsRegular(index_filename));
                    SO::AppendFormat(extra_text, _T(", \"%s\" index created"), PortableFunctions::PathGetFilename(index_filename));
                }

                m_log->WriteFormattedLine(_T("    %-*s    [%s]"),
                                          static_cast<int>(m_maxRepositoryNameLength), index_result.repository_name.c_str(),
                                          extra_text.c_str());
            }
        }

        if( header_written )
            m_log->WriteLine();
    }
}


void Indexer::WriteDuplicatesToLog()
{
    if( m_log == nullptr )
        return;

    std::wstring duplicate_line;
    std::wstring previous_duplicate_key;

    sqlite3_reset(m_stmtFullDuplicateIteratorByKey);

    while( sqlite3_step(m_stmtFullDuplicateIteratorByKey) == SQLITE_ROW )
    {
        if( duplicate_line.empty() )
            m_log->WriteLine(_T("The following duplicate cases were located in your data files:"));

        std::wstring key = FromUtf8WS(sqlite3_column_text(m_stmtFullDuplicateIteratorByKey, 0));

        // write the header if this is the first duplicate with the key
        if( previous_duplicate_key != key )
        {
            const int number_duplicates = sqlite3_column_int(m_stmtFullDuplicateIteratorByKey, 4);

            duplicate_line = FormatTextCS2WS(_T("*** Case [%s] has %d duplicate%s"),
                                             NewlineSubstitutor::NewlineToUnicodeNL(key).c_str(),
                                             number_duplicates, PluralizeWord(number_duplicates));
            m_log->WriteLine();
            m_log->WriteLine(duplicate_line);

            previous_duplicate_key = std::move(key);
        }

        const size_t file_index = sqlite3_column_int(m_stmtFullDuplicateIteratorByKey, 1);
        const size_t case_index = sqlite3_column_int(m_stmtFullDuplicateIteratorByKey, 2);
        const int64_t line_number = sqlite3_column_int64(m_stmtFullDuplicateIteratorByKey, 3);

        const std::wstring index_text = FormatTextCS2WS(( line_number == 0 ) ? _T("#%d") : _T("#%d Line #%d"),
                                                        static_cast<int>(case_index),
                                                        static_cast<int>(line_number));

        duplicate_line = FormatTextCS2WS(_T("    %-*s    [Case %s]"),
                                         static_cast<int>(m_maxRepositoryNameLength), m_indexResults[file_index].repository_name.c_str(),
                                         index_text.c_str());

        m_log->WriteLine(duplicate_line);
    }

    if( duplicate_line.empty() )
        m_log->WriteLine(_T("No duplicate cases were found in your data files."));

    m_log->WriteLine();
}


void Indexer::CalculateOutputConnectionStrings()
{
    if( m_pff->GetDuplicateCase() == DuplicateCase::List || m_pff->GetDuplicateCase() == DuplicateCase::View )
        return;

    std::wstring filename_mask;

    if( m_pff->GetSingleOutputDataConnectionString().IsFilenamePresent() )
    {
        const std::wstring filename = PortableFunctions::PathGetFilename(m_pff->GetSingleOutputDataConnectionString().GetFilename());

        if( filename.find(IndexerFilenameWildcard) != std::wstring::npos )
        {
            filename_mask = filename;
            m_writeToCombinedFile = false;

            // remove the no-longer-used extension mask
            SO::Replace(filename_mask, _T("<.extension>"), _T(""));
        }
    }

    for( IndexResult& index_result : m_indexResults )
    {
        if( filename_mask.empty() )
        {
            index_result.output_connection_string = m_pff->GetSingleOutputDataConnectionString();
        }

        else
        {
            // use the file mask to create a new filename
            index_result.output_connection_string = index_result.input_connection_string;

            if( index_result.output_connection_string.IsFilenamePresent() )
            {
                const std::wstring directory = PortableFunctions::PathGetDirectory(index_result.output_connection_string.GetFilename());
                const std::wstring filename = PortableFunctions::PathGetFilenameWithoutExtension(index_result.output_connection_string.GetFilename());
                const std::wstring extension = PortableFunctions::PathGetFileExtension(index_result.output_connection_string.GetFilename());

                std::wstring new_filename = filename_mask;
                SO::Replace(new_filename, IndexerFilenameWildcard, filename);

                new_filename = MakeFullPath(directory, PortableFunctions::PathAppendFileExtension(new_filename, extension));

                // this will ensure that any connection strings properties are maintained
                index_result.output_connection_string = ConnectionString(index_result.output_connection_string.ToString(new_filename));
            }
        }
    }
}


void Indexer::ReadDuplicates()
{
    ASSERT(m_numberDuplicates > 0);

    // show a progress bar
    ProcessSummaryDlg process_summary_dlg;

    process_summary_dlg.SetTask([&]
    {
        std::shared_ptr<ProcessSummary> process_summary = m_dictionary->CreateProcessSummary();
        auto case_construction_reporter = std::make_shared<CaseConstructionReporter>(process_summary);
        process_summary_dlg.Initialize(_T("Reading duplicate cases..."), process_summary);

        double progress_bar_value = 0;
        const double progress_bar_increment_value = 100.0 / m_numberDuplicates;

        std::unique_ptr<DataRepository> input_repository;
        bool using_text_based_repository = false;
        size_t current_file_index = SIZE_MAX;

        sqlite3_reset(m_stmtReadDuplicatesIterator);

        while( sqlite3_step(m_stmtReadDuplicatesIterator) == SQLITE_ROW )
        {
            const size_t file_index = sqlite3_column_int(m_stmtReadDuplicatesIterator, 0);

            // open a different file if necessary
            if( current_file_index != file_index )
            {
                // if a text-based repository has duplicates, it can't be opened using read only mode, so open in batch mode
                using_text_based_repository = DataRepositoryHelpers::DoesTypeUseIndexableText(m_indexResults[file_index].input_connection_string.GetType());

                input_repository = DataRepository::CreateAndOpen(m_fullCaseAccess, m_indexResults[file_index].input_connection_string,
                    using_text_based_repository ? DataRepositoryAccess::BatchInput : DataRepositoryAccess::ReadOnly, DataRepositoryOpenFlag::OpenMustExist);

                process_summary_dlg.SetSource(FormatText(_T("Input Data: %s"), input_repository->GetName(DataRepositoryNameType::Full).GetString()));

                current_file_index = file_index;
            }

            // read the case
            const size_t case_index = sqlite3_column_int(m_stmtReadDuplicatesIterator, 1);
            const double position_in_repository = sqlite3_column_double(m_stmtReadDuplicatesIterator, 2);

            DuplicateInfo duplicate { &m_indexResults[file_index], case_index, 0, m_fullCaseAccess->CreateCase(), true };
            Case& duplicate_case = *duplicate.data_case;

            duplicate_case.SetCaseConstructionReporter(case_construction_reporter);

            if( using_text_based_repository )
            {
                const size_t bytes_for_case = sqlite3_column_int(m_stmtReadDuplicatesIterator, 3);
                duplicate.line_number = sqlite3_column_int64(m_stmtReadDuplicatesIterator, 4);
                assert_cast<IndexableTextRepository&>(*input_repository).ReadCase(duplicate_case, static_cast<int64_t>(position_in_repository), bytes_for_case);
                duplicate_case.LoadAllBinaryData();
            }

            else
            {
                input_repository->ReadCase(duplicate_case, position_in_repository);
            }

            // add the case to the duplicate cases map
            auto duplicate_lookup = m_duplicates.find(CS2WS(duplicate_case.GetKey()));

            if( duplicate_lookup == m_duplicates.cend() )
            {
                m_duplicates.try_emplace(CS2WS(duplicate_case.GetKey()), std::vector<DuplicateInfo> { duplicate });
            }

            else
            {
                duplicate_lookup->second.emplace_back(duplicate);
            }

            // update the progress bar after each duplicate read
            if( process_summary_dlg.IsCanceled() )
                throw UserCanceledException();

            progress_bar_value += progress_bar_increment_value;
            process_summary->SetPercentSourceRead(progress_bar_value);
            process_summary_dlg.SetKey(duplicate_case.GetKey());
        }
    });

    process_summary_dlg.DoModal();

    process_summary_dlg.RethrowTaskExceptions();
}


void Indexer::ChooseDuplicatesToKeep()
{
    ASSERT(!m_duplicates.empty());
    size_t duplicate_index = 0;

    for( auto& [key, case_duplicates] : m_duplicates )
    {
        ASSERT(case_duplicates.size() >= 2);

        bool show_dialog = true;
        ++duplicate_index;

        // if only prompting when different, check if there are differences
        if( m_pff->GetDuplicateCase() == DuplicateCase::PromptIfDifferent )
        {
            bool differences_exist = false;

            for( size_t i = 1; i < case_duplicates.size(); ++i )
            {
                if( !CaseEquals(*case_duplicates[i - 1].data_case, *case_duplicates[i].data_case) )
                {
                    differences_exist = true;
                    break;
                }
            }

            // if no differences, mark all but the first to be removed
            if( !differences_exist )
            {
                for( auto itr = case_duplicates.begin() + 1; itr != case_duplicates.end(); ++itr )
                    itr->keep = false;

                show_dialog = false;
            }
        }

        // show the duplicates dialog
        if( show_dialog )
            ChooseDuplicate(case_duplicates, duplicate_index, m_duplicates.size());


        // update the database with information about cases to be removed
        for( const DuplicateInfo& duplicate : case_duplicates )
        {
            if( !duplicate.keep )
            {
                sqlite3_reset(m_stmtUpdateCaseDoNotKeepByIndex);
                sqlite3_bind_int(m_stmtUpdateCaseDoNotKeepByIndex, 1, static_cast<int>(duplicate.index_result->file_index));
                sqlite3_bind_int(m_stmtUpdateCaseDoNotKeepByIndex, 2, static_cast<int>(duplicate.case_index));

                if( sqlite3_step(m_stmtUpdateCaseDoNotKeepByIndex) != SQLITE_DONE )
                    throw IndexerDatabaseException();
            }
        }

        // free up any memory associated with the cases
        case_duplicates.clear();
    }
}


bool Indexer::CaseEquals(const Case& case1, const Case& case2)
{
    ASSERT(case1.GetKey() == case2.GetKey());
    return ( case1.GetRootCaseLevel() == case2.GetRootCaseLevel() );
}


void Indexer::WriteCases()
{
    // if only one file is being indexed, don't write a combined file (which will always write a file
    // even when there are no duplicates)
    if( m_indexResults.size() == 1 )
        m_writeToCombinedFile = false;

    // ensure that output filenames are unique
    auto verify_output_connection_string_is_unique = [&](const ConnectionString& output_connection_string)
    {
        for( const IndexResult& index_result : m_indexResults )
        {
            if( index_result.input_connection_string.Equals(output_connection_string) )
                throw CSProException(_T("You cannot output to the same data file as the input: %s"), output_connection_string.ToString().c_str());
        }
    };

    ASSERT(m_log != nullptr);

    auto write_output_file_information = [&](const std::unique_ptr<DataRepository>& output_repository,
                                             size_t number_cases_written, size_t number_duplicates_kept,
                                             size_t number_duplicates_skipped)
    {
        std::wstring extra_text = FormatTextCS2WS(_T("%d case%s written"),
                                                  static_cast<int>(number_cases_written), PluralizeWord(number_cases_written));

        if( number_duplicates_kept > 0 )
        {
            SO::AppendFormat(extra_text, _T(", %d duplicate case%s kept"),
                                         static_cast<int>(number_duplicates_kept), PluralizeWord(number_duplicates_kept));
        }

        if( number_duplicates_skipped > 0 )
        {
            SO::AppendFormat(extra_text, _T(", %d duplicate case%s skipped"),
                                         static_cast<int>(number_duplicates_skipped), PluralizeWord(number_duplicates_skipped));
        }

        m_log->WriteFormattedLine(_T("    %-*s    [%s]"),
                                  static_cast<int>(m_maxRepositoryNameLength), output_repository->GetName(DataRepositoryNameType::Full).GetString(),
                                  extra_text.c_str());
    };


    // write to a combined file
    if( m_writeToCombinedFile )
    {
        verify_output_connection_string_is_unique(m_pff->GetSingleOutputDataConnectionString());

        std::unique_ptr<DataRepository> combined_output_repository = DataRepository::CreateAndOpen(m_fullCaseAccess, m_pff->GetSingleOutputDataConnectionString(),
            DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew);

        size_t number_files_processed = 0;
        size_t number_cases_written = 0;
        size_t number_duplicates_kept = 0;
        size_t number_duplicates_skipped = 0;

        for( IndexResult& index_result : m_indexResults )
        {
            ASSERT(index_result.output_connection_string.Equals(m_pff->GetSingleOutputDataConnectionString()));

            // skip files with errors
            if( !index_result.exception_message.empty() )
                continue;

            WriteCases(combined_output_repository, index_result);

            ++number_files_processed;
            number_cases_written += index_result.number_cases_written;
            number_duplicates_kept += index_result.number_duplicates_kept;
            number_duplicates_skipped += index_result.number_duplicates_skipped;
        }

        m_log->WriteFormattedLine(_T("A combined data file was output from %d input file%s:"),
                                  static_cast<int>(number_files_processed), PluralizeWord(number_files_processed));

        write_output_file_information(combined_output_repository, number_cases_written, number_duplicates_kept, number_duplicates_skipped);
    }


    // write to separate files (or to a single file when there was only one input file)
    else
    {
        bool header_written = false;

        for( IndexResult& index_result : m_indexResults )
        {
            // skip files with errors
            if( !index_result.exception_message.empty() )
                continue;

            // there is no need to write out files that have no duplicates
            if( index_result.number_internal_duplicates == 0 && index_result.number_global_duplicates == 0 )
                continue;

            verify_output_connection_string_is_unique(index_result.output_connection_string);

            std::unique_ptr<DataRepository> output_repository = DataRepository::CreateAndOpen(m_fullCaseAccess, index_result.output_connection_string,
                DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew);

            WriteCases(output_repository, index_result);

            if( !header_written )
            {
                m_log->WriteLine(_T("The following data files were output:"));
                header_written = true;
            }

            write_output_file_information(output_repository, index_result.number_cases_written, index_result.number_duplicates_kept, index_result.number_duplicates_skipped);
        }
    }

    m_log->WriteLine();
}


void Indexer::WriteCases(const std::unique_ptr<DataRepository>& output_repository, IndexResult& index_result)
{
    // open the input in batch mode
    std::unique_ptr<DataRepository> input_repository = DataRepository::CreateAndOpen(m_fullCaseAccess,
        index_result.input_connection_string, DataRepositoryAccess::BatchInput, DataRepositoryOpenFlag::OpenMustExist);

    // show a progress bar
    ProcessSummaryDlg process_summary_dlg;

    process_summary_dlg.SetTask([&]
    {
        std::shared_ptr<ProcessSummary> process_summary = m_dictionary->CreateProcessSummary();
        process_summary_dlg.Initialize(_T("Reading and writing cases..."), process_summary);
        process_summary_dlg.SetSource(FormatText(_T("Input Data: %s"), input_repository->GetName(DataRepositoryNameType::Full).GetString()));

        size_t progress_bar_update_counter = ProgressBarCaseUpdateFrequency;

        auto input_case_iterator = input_repository->CreateCaseIterator(CaseIterationMethod::SequentialOrder, CaseIterationOrder::Ascending);
        size_t case_index = 0;

        size_t next_duplicate_case_index = SIZE_MAX;
        bool next_duplicate_keep = false;


        auto get_next_duplicate_information = [&]()
        {
            sqlite3_reset(m_stmtWriteCaseIterator);
            sqlite3_bind_int(m_stmtWriteCaseIterator, 1, static_cast<int>(index_result.file_index));
            sqlite3_bind_int(m_stmtWriteCaseIterator, 2, static_cast<int>(case_index));

            if( sqlite3_step(m_stmtWriteCaseIterator) == SQLITE_DONE )
            {
                next_duplicate_case_index = SIZE_MAX;
            }

            else
            {
                next_duplicate_case_index = sqlite3_column_int(m_stmtWriteCaseIterator, 0);
                next_duplicate_keep = ( sqlite3_column_int(m_stmtWriteCaseIterator, 1) == 1 );
            }
        };

        get_next_duplicate_information();


        // read (and potentially write) each case
        std::unique_ptr<Case> data_case = m_fullCaseAccess->CreateCase();
        data_case->SetCaseConstructionReporter(std::make_unique<CaseConstructionReporter>(process_summary));

        while( input_case_iterator->NextCase(*data_case) )
        {
            bool write_case = true;

            if( ++case_index == next_duplicate_case_index )
            {
                if( !next_duplicate_keep )
                {
                    write_case = false;
                    ++index_result.number_duplicates_skipped;
                }

                else
                {
                    ++index_result.number_duplicates_kept;

                    // when processing the first kept case, mark all other cases as to not be kept
                    if( m_pff->GetDuplicateCase() == DuplicateCase::KeepFirst )
                    {
                        sqlite3_reset(m_stmtUpdateCaseDoNotKeepByKey);
                        sqlite3_bind_text(m_stmtUpdateCaseDoNotKeepByKey, 1, ToUtf8(data_case->GetKey()), -1, SQLITE_TRANSIENT);

                        if( sqlite3_step(m_stmtUpdateCaseDoNotKeepByKey) != SQLITE_DONE )
                            throw IndexerDatabaseException();
                    }
                }

                get_next_duplicate_information();
            }

            if( write_case )
            {
                output_repository->WriteCase(*data_case);
                ++index_result.number_cases_written;
            }


            // update the progress bar
            if( process_summary_dlg.IsCanceled() )
                throw UserCanceledException();

            if( --progress_bar_update_counter == 0 )
            {
                process_summary->SetPercentSourceRead(input_case_iterator->GetPercentRead());
                process_summary_dlg.SetKey(data_case->GetKey());
                progress_bar_update_counter = ProgressBarCaseUpdateFrequency;
            }
        }
    });

    process_summary_dlg.DoModal();

    process_summary_dlg.RethrowTaskExceptions();
}
