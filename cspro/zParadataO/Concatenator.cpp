#include "stdafx.h"
#include "Concatenator.h"
#include <zUtilO/Interapp.h>
#include <SQLite/SQLiteHelpers.h>


namespace
{
    namespace ConcatMessages
    {
        constexpr const TCHAR* StartingOperation = _T("Starting operation");
        constexpr const TCHAR* AnalyzingEvents = _T("Analyzing events");
        constexpr const TCHAR* ConcatenatingEvents = _T("Concatenating events");
        constexpr const TCHAR* WorkingInFile = _T("Working in file %s");
    }

    namespace ConcatErrors
    {
        constexpr const TCHAR* CreateWorkingDatabase = _T("Could not create the working database");
        constexpr const TCHAR* CreateTable = _T("Could not create a table");
        constexpr const TCHAR* CreatePreparedStatement = _T("Could not create a prepared statement");
        constexpr const TCHAR* Transaction = _T("Could not start or end a transaction");
        constexpr const TCHAR* Insert = _T("Could not insert a row into the database");
        constexpr const TCHAR* Pragma = _T("Could not set a pragma");
        constexpr const TCHAR* Query = _T("Could not execute a query");
        constexpr const TCHAR* UnexpectedOutcome = _T("The program's operation encountered an unexpected outcome");
    }

    namespace ConcatConstants
    {
        constexpr size_t MaximumOpenFiles = 200;
        constexpr size_t EventsPerTransaction = 1000;
        constexpr double AnalyzingEventsProgressPercent = 0.10;
    }

    namespace ConcatSqlStatements
    {
        constexpr const char* CountEvents = "SELECT COUNT(*) FROM `event`;";

        // working application instances
        constexpr const char* CreateWorkingApplicationInstanceTable =
            "CREATE TABLE `working_application_instance` ( `uuid` TEXT PRIMARY KEY, `id` INTEGER, `time` REAL, `file_index` INTEGER );";

        constexpr const char* InsertWorkingApplicationInstance =
            "INSERT OR IGNORE INTO `working_application_instance` ( `uuid`, `id`, `time`, `file_index` ) VALUES ( ?, ?, ?, ? );";

        constexpr const char* QueryApplicationInstancesFromLog =
            "SELECT `application_instance`.`uuid`, `application_instance`.`id`, `event`.`time` "
            "FROM `application_instance` "
            "JOIN `event` ON `event`.`application_instance` = `application_instance`.`id` "
            "JOIN `application_event` ON `application_event`.`id` = `event`.`id` "
            "WHERE `application_event`.`action` = 1;";

        constexpr const char* CountWorkingApplicationInstances =
            "SELECT COUNT(*) FROM `working_application_instance` WHERE `file_index` >= ?;";

        constexpr const char* QueryWorkingApplicationInstances =
            "SELECT `id`, `file_index` FROM `working_application_instance`  WHERE `file_index` >= ? ORDER BY `time`;";

        // working associated rows
        constexpr const char* CreateWorkingAssociatedRowTable =
            "CREATE TABLE `working_associated_row` ( `file_id` INTEGER, `table_id` INTEGER, `input_id` INTEGER, `output_id` INTEGER );";

        constexpr const char* CreateWorkingAssociatedRowIndex =
            "CREATE INDEX `working_associated_row_index` ON `working_associated_row` ( `file_id`, `table_id`, `input_id` );";

        constexpr const char* InsertWorkingAssociatedRow =
            "INSERT INTO `working_associated_row` ( `file_id`, `table_id`, `input_id`, `output_id` ) VALUES ( ?, ?, ?, ? );";

        constexpr const char* QueryWorkingAssociatedRow =
            "SELECT `output_id` FROM `working_associated_row` WHERE `file_id` = ? AND `table_id` = ? AND `input_id` = ?;";

        // metadata
        constexpr const char* QueryColumnMetadata =
            "SELECT `metadata_column_info`.`column`, `metadata_column_info`.`type`, `metadata_column_info`.`nullable` "
            "FROM `metadata_column_info` "
            "JOIN `metadata_table_info` ON `metadata_column_info`.`metadata_table_info` = `metadata_table_info`.`id` "
            "WHERE `metadata_table_info`.`table` = ? "
            "ORDER BY `metadata_column_info`.`id`;";
    }
}


namespace Paradata
{
    struct ConcatColumn
    {
        Table::ColumnEntry columnEntry;
        ConcatTable* pAssociatedConcatTable = nullptr;
    };

    struct ConcatTable
    {
        const TableDefinition& table_definition;
        std::vector<ConcatColumn> aColumns;
        bool bAutoIncrementId = false;
        bool bAutoIncrementIfUniqueWithNulls = false;
        sqlite3_stmt* pOutputInsertStmt = nullptr;
        std::vector<sqlite3_stmt*> pOutputSelectStmts;
        std::vector<sqlite3_stmt*> pInputSelectStmts;
    };
}


Paradata::Concatenator::Concatenator()
    :   m_OutputDb(nullptr),
        m_bOutputDbIsCurrentlyOpenParadataLog(false),
        m_WorkingDb(nullptr),
        m_stmtInsertWorkingApplicationInstance(nullptr),
        m_stmtInsertWorkingAssociatedRow(nullptr),
        m_stmtQueryWorkingAssociatedRow(nullptr),
        m_pBaseEventConcatTable(nullptr),
        m_iStartingFileIndex(0),
        m_iCurrentFileIndex(0),
        m_dOperationProgressBarValue(0),
        m_dTotalProgressBarValue(0)
{
}


Paradata::Concatenator::~Concatenator()
{
    Cleanup();
}


void Paradata::Concatenator::Cleanup()
{
    // input databases
    CloseInputDatabases();

    // output database
    for( auto tableItr = m_apConcatTables.begin(); tableItr != m_apConcatTables.end(); tableItr++ )
    {
        ConcatTable* pConcatTable = *tableItr;

        safe_sqlite3_finalize(pConcatTable->pOutputInsertStmt);

        for( auto stmtItr = pConcatTable->pOutputSelectStmts.begin(); stmtItr != pConcatTable->pOutputSelectStmts.end(); stmtItr++ )
            safe_sqlite3_finalize(*stmtItr);

        delete pConcatTable;
    }

    m_apConcatTables.clear();

    if( m_bOutputDbIsCurrentlyOpenParadataLog )
    {
        RestoreDatabasePragmas();
    }

    else 
    {
        if( m_OutputDb != nullptr )
            sqlite3_close(m_OutputDb);

        m_OutputDb = nullptr;
    }

    // working database
    safe_sqlite3_finalize(m_stmtInsertWorkingApplicationInstance);
    safe_sqlite3_finalize(m_stmtInsertWorkingAssociatedRow);
    safe_sqlite3_finalize(m_stmtQueryWorkingAssociatedRow);

    if( m_WorkingDb != nullptr )
    {
        sqlite3_close(m_WorkingDb);
        m_WorkingDb = nullptr;
    }
}


void Paradata::Concatenator::CloseInputDatabases()
{
    for( auto tableItr = m_apConcatTables.begin(); tableItr != m_apConcatTables.end(); tableItr++ )
    {
        ConcatTable* pConcatTable = *tableItr;

        for( auto stmtItr = pConcatTable->pInputSelectStmts.begin(); stmtItr != pConcatTable->pInputSelectStmts.end(); stmtItr++ )
            safe_sqlite3_finalize(*stmtItr);

        pConcatTable->pInputSelectStmts.clear();
    }

    for( auto dbItr = m_inputDbs.begin(); dbItr != m_inputDbs.end(); dbItr++ )
    {
        if( *dbItr != nullptr )
            sqlite3_close(*dbItr);
    }

    m_inputDbs.clear();
}


// default implementations of the virtual functions
void Paradata::Concatenator::OnInputProcessedSuccess(const std::variant<std::wstring, sqlite3*>& /*filename_or_database*/, int64_t /*iEventsProcessed*/)
{
}

void Paradata::Concatenator::OnInputProcessedError(NullTerminatedString /*input_filename*/, const std::wstring& error_message)
{
    throw CSProException(error_message);
}

void Paradata::Concatenator::OnProgressUpdate(const CString& /*csOperationMessage*/, int /*iOperationPercent*/, const CString& /*csTotalMessage*/, int /*iTotalPercent*/)
{
}

bool Paradata::Concatenator::UserRequestsCancellation()
{
    return false;
}


template<typename T>
T Paradata::Concatenator::ExecuteSingleQuery(sqlite3* db, const char* query, std::optional<int> argument/* = std::nullopt*/)
{
    sqlite3_stmt* stmt = nullptr;

    if( sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreatePreparedStatement);

    if( argument.has_value() )
        sqlite3_bind_int(stmt, 1, *argument);

    if( sqlite3_step(stmt) != SQLITE_ROW )
        throw CSProException(ConcatErrors::Query);

    T value;

    if constexpr(std::is_same_v<T, int64_t>)
        value = sqlite3_column_int64(stmt, 0);

    else
        value = FromUtf8(sqlite3_column_text(stmt, 0));

    safe_sqlite3_finalize(stmt);

    return value;
}


int64_t Paradata::Concatenator::GetNumberEvents(sqlite3* db)
{
    return ExecuteSingleQuery<int64_t>(db, ConcatSqlStatements::CountEvents);
}


void Paradata::Concatenator::Run(std::variant<std::wstring, sqlite3*> output_filename_or_database, const std::set<std::wstring>& paradata_log_filenames)
{
    OnProgressUpdate(ConcatMessages::StartingOperation,0,ConcatMessages::StartingOperation,0);

    bool bOutputIsAlsoAnInput = false;

    if( std::holds_alternative<std::wstring>(output_filename_or_database) )
    {
        if( SO::IsBlank(std::get<std::wstring>(output_filename_or_database)) )
            throw CSProException("You must specify the filename of the output log");
    }

    else
    {
        m_bOutputDbIsCurrentlyOpenParadataLog = true;

        m_OutputDb = std::get<sqlite3*>(output_filename_or_database);
        ASSERT(m_OutputDb != nullptr);            

        bOutputIsAlsoAnInput = true;
    }

    // create batches of files to concatenate
    std::vector<std::vector<std::wstring>> input_filenames;
    size_t iNumberInputs = 0;

    for( const std::wstring& paradata_log_filename : paradata_log_filenames )
    {
        if( !PortableFunctions::FileIsRegular(paradata_log_filename) )
        {
            OnInputProcessedError(paradata_log_filename, _T("Could not find the input log"));
        }

        else if( std::holds_alternative<std::wstring>(output_filename_or_database) &&
                 SO::EqualsNoCase(std::get<std::wstring>(output_filename_or_database), paradata_log_filename) )
        {
            bOutputIsAlsoAnInput = true;
        }

        else
        {
            if( input_filenames.empty() || input_filenames.back().size() == ConcatConstants::MaximumOpenFiles )
                input_filenames.emplace_back();

            iNumberInputs++;
            input_filenames.back().emplace_back(paradata_log_filename);
        }
    }

    if( iNumberInputs == 0 )
        throw CSProException("You must specify at least one input log");


    // initialize the output file and working database
    if( !m_bOutputDbIsCurrentlyOpenParadataLog )
        CreateOutputFile(std::get<std::wstring>(output_filename_or_database), bOutputIsAlsoAnInput);

    SetupOutputConcatTables();

    if( bOutputIsAlsoAnInput )
    {
        OnInputProcessedSuccess(output_filename_or_database, GetNumberEvents(m_OutputDb));
        SetDatabasePragmas(m_OutputDb, false, true);
    }

    else // this will speed up the output
    {
        SetDatabasePragmas(m_OutputDb, true, true);
    }

    CreateWorkingDatabase();

    // add the application instances from the output file (if it is also an input)
    if( bOutputIsAlsoAnInput )
    {
        SetProgressUpdateInFile(output_filename_or_database, ConcatMessages::AnalyzingEvents);
        AddApplicationInstancesToWorkingDatabase(m_OutputDb);
    }


    // run the concatenation on each batch
    m_iStartingFileIndex = 1;
    double dTotalProgressBarFileStep = 100 / iNumberInputs;

    for( const auto& this_input_filenames : input_filenames )
    {
        RunBatch(this_input_filenames, dTotalProgressBarFileStep);
        m_iStartingFileIndex += this_input_filenames.size();
    }

    // end the concatenation
    Cleanup();
}


void Paradata::Concatenator::CreateOutputFile(NullTerminatedString output_filename, bool bOutputIsAlsoAnInput)
{
    if( !bOutputIsAlsoAnInput && PortableFunctions::FileExists(output_filename) && !PortableFunctions::FileDelete(output_filename) )
        throw CSProException("Could not delete the output log");

    // opening the file via the Log class will create all the tables needed in the output file
    m_OutputDb = Log::GetDatabaseForTool(output_filename, true);
}


void Paradata::Concatenator::CreateWorkingDatabase()
{
    // in release a temporary database will be created (in memory, and flushed to disk if necessary)
    std::wstring working_db_filename = DebugMode() ? GetUniqueTempFilename(_T("paraconcat.db")) :
                                                     std::wstring();
                                                     
    if( sqlite3_open(ToUtf8(working_db_filename), &m_WorkingDb) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreateWorkingDatabase);

    SetDatabasePragmas(m_WorkingDb, true, true);

    const char* ExecSql[] =
    {
        ConcatSqlStatements::CreateWorkingApplicationInstanceTable,
        ConcatSqlStatements::CreateWorkingAssociatedRowTable,
        ConcatSqlStatements::CreateWorkingAssociatedRowIndex
    };

    for( int i = 0; i < _countof(ExecSql); i++ )
    {
        if( sqlite3_exec(m_WorkingDb,ExecSql[i],nullptr,nullptr,nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreateTable);
    }

    const void* PrepareSql[] =
    {
        (const void*)ConcatSqlStatements::InsertWorkingApplicationInstance, &m_stmtInsertWorkingApplicationInstance,
        (const void*)ConcatSqlStatements::InsertWorkingAssociatedRow, &m_stmtInsertWorkingAssociatedRow,
        (const void*)ConcatSqlStatements::QueryWorkingAssociatedRow, &m_stmtQueryWorkingAssociatedRow
    };

    for( int i = 0; i < _countof(PrepareSql); i += 2 )
    {
        if( sqlite3_prepare_v2(m_WorkingDb,(const char*)PrepareSql[i],-1,(sqlite3_stmt**)PrepareSql[i + 1],nullptr) != SQLITE_OK  )
            throw CSProException(ConcatErrors::CreatePreparedStatement);
    }
}


void Paradata::Concatenator::SetDatabasePragma(sqlite3* db, const TCHAR* pragma, const TCHAR* value, bool add_to_cache)
{
    // cache the current pragma setting when working with a paradata log that will continue to be used post-concatenation
    if( add_to_cache && m_bOutputDbIsCurrentlyOpenParadataLog )
    {
        CString query_pragma_sql = FormatText(_T("PRAGMA %s;"), pragma);
        CString current_setting = ExecuteSingleQuery<CString>(db, ToUtf8(query_pragma_sql));
        m_pragmaSettingsToRestore.emplace_back(pragma, current_setting);
    }

    CString set_pragma_sql = FormatText(_T("PRAGMA %s = %s;"), pragma, value);

    if( sqlite3_exec(db, ToUtf8(set_pragma_sql), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::Pragma);
}


void Paradata::Concatenator::SetDatabasePragmas(sqlite3* db, bool set_journal_mode_off, bool set_synchronous_off)
{
    const TCHAR* const JournalMode     = _T("journal_mode");
    const TCHAR* const SynchronousMode = _T("synchronous");

    if( set_journal_mode_off )
        SetDatabasePragma(db, JournalMode, _T("off"), true);

    if( set_synchronous_off )
        SetDatabasePragma(db, SynchronousMode, _T("off"), true);
}


void Paradata::Concatenator::RestoreDatabasePragmas()
{
    ASSERT(m_bOutputDbIsCurrentlyOpenParadataLog && m_OutputDb != nullptr);

    for( const auto& [pragma, value] : m_pragmaSettingsToRestore )
        SetDatabasePragma(m_OutputDb, pragma, value, false);
}


void Paradata::Concatenator::SetProgressUpdateInFile(const std::variant<std::wstring, sqlite3*>& filename_or_database, CString csTotalMessage)
{
    CString progress_text = FormatText(ConcatMessages::WorkingInFile,
        std::holds_alternative<std::wstring>(filename_or_database) ? PortableFunctions::PathGetFilename(std::get<std::wstring>(filename_or_database)) :
                                                                     _T("open paradata log"));

    OnProgressUpdate(progress_text, (int)m_dOperationProgressBarValue, csTotalMessage, (int)m_dTotalProgressBarValue);
}


sqlite3* Paradata::Concatenator::OpenInputFile(NullTerminatedString input_filename)
{
    return Log::GetDatabaseForTool(input_filename, false);
}


void Paradata::Concatenator::RunBatch(const std::vector<std::wstring>& input_filenames, double dTotalProgressBarFileStep)
{
    double dTotalProgressBarAnalyzeStep = dTotalProgressBarFileStep * ConcatConstants::AnalyzingEventsProgressPercent;
    double dOperationProgressBarAnalyzeStep = 100.0 / input_filenames.size();
    m_dOperationProgressBarValue = 0;

    // open the files and add the application instances
    for( m_iCurrentFileIndex = 0; m_iCurrentFileIndex < (int)input_filenames.size(); ++m_iCurrentFileIndex )
    {
        SetProgressUpdateInFile(input_filenames[m_iCurrentFileIndex], ConcatMessages::AnalyzingEvents);

        sqlite3* db = nullptr;

        try
        {
            db = OpenInputFile(input_filenames[m_iCurrentFileIndex]);
            AddApplicationInstancesToWorkingDatabase(db);
        }

        catch( const CSProException& exception )
        {
            OnInputProcessedError(input_filenames[m_iCurrentFileIndex], exception.GetErrorMessage());
            db = nullptr;
        }

        m_inputDbs.emplace_back(db);

        if( UserRequestsCancellation() )
            throw UserCanceledException();

        m_dOperationProgressBarValue += dOperationProgressBarAnalyzeStep;
        m_dTotalProgressBarValue += dTotalProgressBarAnalyzeStep;
    }


    // run through the (non-duplicative) application instances, in timestamp order, and
    // insert events from those application instances in the output file
    int64_t iNumberApplicationInterfaces = ExecuteSingleQuery<int64_t>(m_WorkingDb,
        ConcatSqlStatements::CountWorkingApplicationInstances, m_iStartingFileIndex);

    std::vector<int64_t> aNumberApplicationEventsPerFile(m_inputDbs.size(),0);

    if( iNumberApplicationInterfaces > 0 )
    {
        double dTotalProgressBarConcatenatingStep = ( dTotalProgressBarFileStep - dTotalProgressBarAnalyzeStep ) *
            input_filenames.size() / iNumberApplicationInterfaces;
        double dOperationProgressBarConcatenatingStep = 100.0 / iNumberApplicationInterfaces;
        m_dOperationProgressBarValue = 0;

        sqlite3_stmt* stmt = nullptr;

        if( sqlite3_prepare_v2(m_WorkingDb,ConcatSqlStatements::QueryWorkingApplicationInstances,-1,&stmt,nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreatePreparedStatement);

        sqlite3_bind_int(stmt,1,m_iStartingFileIndex);

        while( sqlite3_step(stmt) == SQLITE_ROW )
        {
            long lApplicationInstanceId = (long)sqlite3_column_int64(stmt,0);
            m_iCurrentFileIndex = sqlite3_column_int(stmt,1) - m_iStartingFileIndex;

            if( m_inputDbs[m_iCurrentFileIndex] != nullptr )
            {
                SetProgressUpdateInFile(input_filenames[m_iCurrentFileIndex], ConcatMessages::ConcatenatingEvents);

                aNumberApplicationEventsPerFile[m_iCurrentFileIndex] += ConcatenateEvents(lApplicationInstanceId);

                if( UserRequestsCancellation() )
                    throw UserCanceledException();
            }

            m_dOperationProgressBarValue += dOperationProgressBarConcatenatingStep;
            m_dTotalProgressBarValue += dTotalProgressBarConcatenatingStep;
        }

        safe_sqlite3_finalize(stmt);
    }


    // wrap up work on this batch
    for( size_t i = 0; i < m_inputDbs.size(); i++ )
    {
        if( m_inputDbs[i] != nullptr )
            OnInputProcessedSuccess(input_filenames[i], aNumberApplicationEventsPerFile[i]);
    }

    CloseInputDatabases();
}


void Paradata::Concatenator::AddApplicationInstancesToWorkingDatabase(sqlite3* db)
{
    // query the application instances
    sqlite3_stmt* stmt = nullptr;

    if( sqlite3_prepare_v2(db,ConcatSqlStatements::QueryApplicationInstancesFromLog,-1,&stmt,nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreatePreparedStatement);

    while( sqlite3_step(stmt) == SQLITE_ROW )
    {
        CString csUuid = FromUtf8(sqlite3_column_text(stmt,0));
        long lApplicationInstanceId = (long)sqlite3_column_int64(stmt,1);
        double dTimestamp = sqlite3_column_double(stmt,2);

        // update the working database
        sqlite3_reset(m_stmtInsertWorkingApplicationInstance);

        sqlite3_bind_text(m_stmtInsertWorkingApplicationInstance,1,ToUtf8(csUuid),-1,SQLITE_TRANSIENT);
        sqlite3_bind_int64(m_stmtInsertWorkingApplicationInstance,2,lApplicationInstanceId);
        sqlite3_bind_double(m_stmtInsertWorkingApplicationInstance,3,dTimestamp);
        sqlite3_bind_int(m_stmtInsertWorkingApplicationInstance,4,m_iCurrentFileIndex + m_iStartingFileIndex);

        if( sqlite3_step(m_stmtInsertWorkingApplicationInstance) != SQLITE_DONE )
            throw CSProException(ConcatErrors::Insert);
    }

    safe_sqlite3_finalize(stmt);
}


void Paradata::Concatenator::SetupOutputConcatTables()
{
    // first add all of the tables
    for( size_t i = ParadataTable_FirstNonMetadataTableIndex; i < ParadataTable_NumberTables; ++i )
    {
        ConcatTable* pConcatTable = new ConcatTable { GetTableDefinition(static_cast<ParadataTable>(i)) };
        m_apConcatTables.push_back(pConcatTable);

        if( pConcatTable->table_definition.table_code > 0 ) // an event
            m_mapEventTypeToConcatTable[pConcatTable->table_definition.table_code] = pConcatTable;

        else if( pConcatTable->table_definition.type == ParadataTable::BaseEvent )
            m_pBaseEventConcatTable = pConcatTable;
    }

    // then read all of the column information from the database
    sqlite3_stmt* stmt = nullptr;

    if( sqlite3_prepare_v2(m_OutputDb,ConcatSqlStatements::QueryColumnMetadata,-1,&stmt,nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreatePreparedStatement);

    for( auto tableItr = m_apConcatTables.begin(); tableItr != m_apConcatTables.end(); tableItr++ )
    {
        ConcatTable* pConcatTable = *tableItr;

        sqlite3_reset(stmt);

        sqlite3_bind_text(stmt,1,ToUtf8(pConcatTable->table_definition.name),-1,SQLITE_TRANSIENT);

        while( sqlite3_step(stmt) == SQLITE_ROW )
        {
            pConcatTable->aColumns.push_back(ConcatColumn());
            ConcatColumn& concatColumn = pConcatTable->aColumns.back();

            concatColumn.columnEntry.name = FromUtf8(sqlite3_column_text(stmt, 0));
            concatColumn.columnEntry.type = Table::StringToColumnType(FromUtf8(sqlite3_column_text(stmt, 1)));
            concatColumn.columnEntry.nullable = ( sqlite3_column_int(stmt, 2) == 1 );

            // check if this column's value points to another table
            if( concatColumn.columnEntry.type == Table::ColumnType::Long )
            {
                for( auto secondTableItr = m_apConcatTables.begin(); secondTableItr != m_apConcatTables.end(); secondTableItr++ )
                {
                    int iTableNamePos = concatColumn.columnEntry.name.Find((*secondTableItr)->table_definition.name);

                    // if the table name is at the end, it's a match
                    if( ( iTableNamePos >= 0 ) &&
                        ( ( iTableNamePos + (int)_tcslen((*secondTableItr)->table_definition.name) ) == concatColumn.columnEntry.name.GetLength() ) )
                    {
                        concatColumn.pAssociatedConcatTable = *secondTableItr;
                        break;
                    }
                }
            }

            pConcatTable->bAutoIncrementId =
                ( pConcatTable->table_definition.insert_type == InsertType::AutoIncrement ) ||
                ( pConcatTable->table_definition.insert_type == InsertType::AutoIncrementIfUnique );

            if( concatColumn.columnEntry.nullable && ( pConcatTable->table_definition.insert_type == InsertType::AutoIncrementIfUnique ) )
                pConcatTable->bAutoIncrementIfUniqueWithNulls = true;
        }

        if( pConcatTable->aColumns.size() == 0 )
            throw CSProException(ConcatErrors::UnexpectedOutcome);
    }

    safe_sqlite3_finalize(stmt);
}


sqlite3_stmt* Paradata::Concatenator::GetInputSelectStatement(ConcatTable* pConcatTable,LPCTSTR lpszWhereColumnName)
{
    sqlite3_stmt** pStmt = nullptr;

    // see if the statement already has been created
    if( pConcatTable->pInputSelectStmts.size() <= (size_t)m_iCurrentFileIndex )
        pConcatTable->pInputSelectStmts.resize(m_iCurrentFileIndex + 1,nullptr);

    pStmt = &pConcatTable->pInputSelectStmts[m_iCurrentFileIndex];

    // if so, reset it and use it
    if( *pStmt != nullptr )
        sqlite3_reset(*pStmt);

    // if not, create it
    else
    {
        CString csSelectSql = _T("SELECT `id`");

        for( size_t i = 0; i < pConcatTable->aColumns.size(); i++ )
            csSelectSql.AppendFormat(_T(", `%s`"),(LPCTSTR)pConcatTable->aColumns[i].columnEntry.name);

        csSelectSql.AppendFormat(_T(" FROM `%s` WHERE `%s` = ? ORDER BY `id`;"),
            pConcatTable->table_definition.name,lpszWhereColumnName);

        if( sqlite3_prepare_v2(m_inputDbs[m_iCurrentFileIndex],ToUtf8(csSelectSql),-1,pStmt,nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreatePreparedStatement);
    }

    return *pStmt;
}


sqlite3_stmt* Paradata::Concatenator::GetOutputSelectStatement(ConcatTable* pConcatTable,int iNullValuesFlag)
{
    sqlite3_stmt** pStmt = nullptr;

    // see if the statement already has been created
    if( pConcatTable->pOutputSelectStmts.size() <= (size_t)iNullValuesFlag )
        pConcatTable->pOutputSelectStmts.resize(iNullValuesFlag + 1,nullptr);

    pStmt = &pConcatTable->pOutputSelectStmts[iNullValuesFlag];

    // if so, reset it and use it
    if( *pStmt != nullptr )
        sqlite3_reset(*pStmt);

    // if not, create it
    else
    {
        CString csSelectSql;
        csSelectSql.Format(_T("SELECT `id` FROM `%s` WHERE"),pConcatTable->table_definition.name);

        for( size_t i = 0; i < pConcatTable->aColumns.size(); i++ )
        {
            bool bIsNull = ( iNullValuesFlag & ( 1 << i ) ) != 0;

            csSelectSql.AppendFormat(_T(" %s`%s` %s"),
                ( i == 0 ) ? _T("") : _T("AND "),
                (LPCTSTR)pConcatTable->aColumns[i].columnEntry.name,
                bIsNull ? _T("is null") : _T("= ?"));
        }

        csSelectSql.AppendFormat(_T(" ORDER BY `id`;"));

        if( sqlite3_prepare_v2(m_OutputDb,ToUtf8(csSelectSql),-1,pStmt,nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreatePreparedStatement);
    }

    return *pStmt;
}


sqlite3_stmt* Paradata::Concatenator::GetOutputInsertStatement(ConcatTable* pConcatTable)
{
    if( pConcatTable->pOutputInsertStmt != nullptr )
        sqlite3_reset(pConcatTable->pOutputInsertStmt);

    else // create it
    {
        CString csInsertSql;
        csInsertSql.Format(_T("INSERT INTO `%s` ( %s"),
            pConcatTable->table_definition.name,
            pConcatTable->bAutoIncrementId ? _T("") : _T("`id`"));

        CString csInsertValuesSql = pConcatTable->bAutoIncrementId ? _T("") : _T("?");

        for( size_t i = 0; i < pConcatTable->aColumns.size(); i++ )
        {
            CString csComma = ( pConcatTable->bAutoIncrementId && ( i == 0 ) ) ? _T("") : _T(", ");
            csInsertSql.AppendFormat(_T("%s`%s`"),(LPCTSTR)csComma,(LPCTSTR)pConcatTable->aColumns[i].columnEntry.name);
            csInsertValuesSql.AppendFormat(_T("%s?"),(LPCTSTR)csComma);
        }

        csInsertSql.AppendFormat(_T(" ) VALUES ( %s );"),(LPCTSTR)csInsertValuesSql);

        if( sqlite3_prepare_v2(m_OutputDb,ToUtf8(csInsertSql),-1,&pConcatTable->pOutputInsertStmt,nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreatePreparedStatement);
    }

    return pConcatTable->pOutputInsertStmt;
}


void Paradata::Concatenator::ManageTransaction(int* piEventsUntilNextTransaction)
{
    const char* lpszSqlStatement = SqlStatements::EndTransaction;

    if( piEventsUntilNextTransaction != nullptr )
    {
        if( *piEventsUntilNextTransaction == 0 ) // end the current transaction
            ManageTransaction(nullptr);

        lpszSqlStatement = SqlStatements::BeginTransaction;
    }

    if( sqlite3_exec(m_OutputDb,lpszSqlStatement,nullptr,nullptr,nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::Transaction);
}


int64_t Paradata::Concatenator::ConcatenateEvents(long lApplicationInstanceId)
{
    int64_t iEvents = 0;

    int iEventsUntilNextTransaction = ConcatConstants::EventsPerTransaction;
    ManageTransaction(&iEventsUntilNextTransaction);

    sqlite3_stmt* stmtSelectBaseEvent = GetInputSelectStatement(m_pBaseEventConcatTable,_T("application_instance"));
    sqlite3_bind_int64(stmtSelectBaseEvent,1,lApplicationInstanceId);

    while( sqlite3_step(stmtSelectBaseEvent) == SQLITE_ROW )
    {
        long lInputBaseEventId = (long)sqlite3_column_int64(stmtSelectBaseEvent,0);
        int iEventType = sqlite3_column_int(stmtSelectBaseEvent,4);

        // process the base event
        long lOutputBaseEventId = 0;
        ConcatenateRow(&lOutputBaseEventId,m_pBaseEventConcatTable,stmtSelectBaseEvent);

        // process the related event
        auto thisEventConcatTableKP = m_mapEventTypeToConcatTable.find(iEventType);

        if( thisEventConcatTableKP == m_mapEventTypeToConcatTable.end() )
            throw CSProException(ConcatErrors::UnexpectedOutcome);

        sqlite3_stmt* stmtSelectRelatedEvent = GetInputSelectStatement(thisEventConcatTableKP->second,_T("id"));
        sqlite3_bind_int64(stmtSelectRelatedEvent,1,lInputBaseEventId);

        if( sqlite3_step(stmtSelectRelatedEvent) != SQLITE_ROW )
            throw CSProException(ConcatErrors::UnexpectedOutcome);

        ConcatenateRow(&lOutputBaseEventId,thisEventConcatTableKP->second,stmtSelectRelatedEvent);

        iEvents++;

        if( --iEventsUntilNextTransaction == 0 )
        {
            if( UserRequestsCancellation() )
                throw UserCanceledException();

            ManageTransaction(&iEventsUntilNextTransaction);
        }
    }

    ManageTransaction(nullptr);

    return iEvents;
}


void Paradata::Concatenator::BindArguments(ConcatTable* pConcatTable,sqlite3_stmt* stmtInput,
    sqlite3_stmt* stmtOutput,int iOutputBindIndex,std::set<ConcatTable*>* pSetCallerQueriedTables/* = nullptr*/)
{
    int iInputReferenceIndex = 1; // to skip over the ID column

    for( auto itr = pConcatTable->aColumns.begin(); itr != pConcatTable->aColumns.end(); itr++ )
    {
        if( itr->columnEntry.nullable && ( sqlite3_column_type(stmtInput,iInputReferenceIndex) == SQLITE_NULL ) )
            sqlite3_bind_null(stmtOutput,iOutputBindIndex);

        else
        {
            switch( itr->columnEntry.type )
            {
                case Table::ColumnType::Boolean:
                case Table::ColumnType::Integer:
                    sqlite3_bind_int(stmtOutput,iOutputBindIndex,sqlite3_column_int(stmtInput,iInputReferenceIndex));
                    break;

                case Table::ColumnType::Long:
                {
                    long lValue = (long)sqlite3_column_int64(stmtInput,iInputReferenceIndex);
                    bool bBindLong = true;

                    if( itr->pAssociatedConcatTable != nullptr )
                    {
                        if( lValue > 0 )
                            lValue = ConcatenateAssociatedRow(lValue,itr->pAssociatedConcatTable,pSetCallerQueriedTables);

                        else // if using an old paradata log, the link may not be defined
                            bBindLong = false;
                    }

                    if( bBindLong )
                        sqlite3_bind_int64(stmtOutput,iOutputBindIndex,lValue);

                    else
                        sqlite3_bind_null(stmtOutput,iOutputBindIndex);

                    break;
                }

                case Table::ColumnType::Double:
                    sqlite3_bind_double(stmtOutput,iOutputBindIndex,sqlite3_column_double(stmtInput,iInputReferenceIndex));
                    break;

                default: // ColumnType::Text
                    sqlite3_bind_text(stmtOutput,iOutputBindIndex,(const char*)sqlite3_column_text(stmtInput,iInputReferenceIndex),-1,SQLITE_TRANSIENT);
                    break;
            }
        }

        iInputReferenceIndex++;
        iOutputBindIndex++;
    }
}


void Paradata::Concatenator::ConcatenateRow(long *plId,ConcatTable* pConcatTable,sqlite3_stmt* stmtInput)
{
    sqlite3_stmt* stmtOutput = GetOutputInsertStatement(pConcatTable);

    // bind the arguments
    int iOutputBindIndex = 1;

    if( !pConcatTable->bAutoIncrementId ) // bind the ID if necessary
        sqlite3_bind_int64(stmtOutput,iOutputBindIndex++,*plId);

    BindArguments(pConcatTable,stmtInput,stmtOutput,iOutputBindIndex);

    // insert the row
    if( sqlite3_step(stmtOutput) != SQLITE_DONE )
        throw CSProException(ConcatErrors::Insert);

    if( pConcatTable->bAutoIncrementId )
        *plId = (long)sqlite3_last_insert_rowid(m_OutputDb);
}


long Paradata::Concatenator::ConcatenateAssociatedRow(long lInputId,ConcatTable* pConcatTable,
    std::set<ConcatTable*>* pSetCallerQueriedTables/* = nullptr*/)
{
    // check if this has already been added to the output
    sqlite3_reset(m_stmtQueryWorkingAssociatedRow);

    sqlite3_bind_int(m_stmtQueryWorkingAssociatedRow,1,m_iCurrentFileIndex + m_iStartingFileIndex);
    sqlite3_bind_int(m_stmtQueryWorkingAssociatedRow,2,(int)pConcatTable->table_definition.type);
    sqlite3_bind_int64(m_stmtQueryWorkingAssociatedRow,3,lInputId);

    if( sqlite3_step(m_stmtQueryWorkingAssociatedRow) == SQLITE_ROW )
        return (long)sqlite3_column_int64(m_stmtQueryWorkingAssociatedRow,0);


    // if not, check if it already exists (for AutoIncrementIfUnique tables) or insert it
    sqlite3_stmt* stmtInput = GetInputSelectStatement(pConcatTable,_T("id"));

    if( pSetCallerQueriedTables != nullptr )
        pSetCallerQueriedTables->insert(pConcatTable);

    // this code is in a loop because of the recursive nature of this function, which can
    // lead to the prepared statement become messed up (for example, if name.parent_name
    // needs to be added, then the query on the initial name would be messed up); there
    // would be ways to do this more efficiently, but this approach works and isn't too
    // inefficient because after a short amount of time all the queries will return above
    long lOutputId = 0;
    bool bMustInsertRow = true;
    bool bKeepProcessing = true;

    while( bKeepProcessing )
    {
        std::set<ConcatTable*> setThisQueriedTables;

        sqlite3_reset(stmtInput);
        sqlite3_bind_int64(stmtInput,1,lInputId);

        if( sqlite3_step(stmtInput) != SQLITE_ROW )
            throw CSProException(ConcatErrors::UnexpectedOutcome);

        // check if it already exists
        if( pConcatTable->table_definition.insert_type == InsertType::AutoIncrementIfUnique )
        {
            int iNullValuesFlag = 0;

            if( pConcatTable->bAutoIncrementIfUniqueWithNulls )
            {
                for( size_t i = 0; i < pConcatTable->aColumns.size(); i++ )
                {
                    if( sqlite3_column_type(stmtInput,i + 1) == SQLITE_NULL ) // + 1 to skip over the ID column
                        iNullValuesFlag |= ( 1 << i );
                }
            }

            sqlite3_stmt* stmtExistsQuery = GetOutputSelectStatement(pConcatTable,iNullValuesFlag);

            BindArguments(pConcatTable,stmtInput,stmtExistsQuery,1,&setThisQueriedTables);

            if( pSetCallerQueriedTables != nullptr )
                pSetCallerQueriedTables->insert(setThisQueriedTables.begin(),setThisQueriedTables.end());

            // if the bound arguments are still valid, we can get out of the loop
            if( setThisQueriedTables.find(pConcatTable) == setThisQueriedTables.end() )
            {
                if( sqlite3_step(stmtExistsQuery) == SQLITE_ROW )
                {
                    lOutputId = (long)sqlite3_column_int64(stmtExistsQuery,0);
                    bMustInsertRow = false;
                }

                bKeepProcessing = false;
            }
        }

        else
            bKeepProcessing = false;
    }

    // it does not exist, so insert it
    if( bMustInsertRow )
        ConcatenateRow(&lOutputId,pConcatTable,stmtInput);

    // and add the reference to the working database
    sqlite3_reset(m_stmtInsertWorkingAssociatedRow);

    sqlite3_bind_int(m_stmtInsertWorkingAssociatedRow,1,m_iCurrentFileIndex + m_iStartingFileIndex);
    sqlite3_bind_int(m_stmtInsertWorkingAssociatedRow,2,(int)pConcatTable->table_definition.type);
    sqlite3_bind_int64(m_stmtInsertWorkingAssociatedRow,3,lInputId);
    sqlite3_bind_int64(m_stmtInsertWorkingAssociatedRow,4,lOutputId);

    if( sqlite3_step(m_stmtInsertWorkingAssociatedRow) != SQLITE_DONE )
        throw CSProException(ConcatErrors::Insert);

    return lOutputId;
}
