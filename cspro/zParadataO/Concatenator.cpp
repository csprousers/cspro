#include "stdafx.h"
#include "Concatenator.h"
#include <zSql/Commands.h>
#include <zUtilO/ExpansiveMap.h>
#include <zUtilO/Interapp.h>

using namespace Paradata;


namespace ConcatMessages
{
    constexpr const char* StartingOperation = "Starting operation";
    constexpr const char* AnalyzingEvents = "Analyzing events";
    constexpr const char* ConcatenatingEvents = "Concatenating events";
    constexpr const char* WorkingInFile = "Working in file %s";
}


namespace ConcatErrors
{
    constexpr const char* CreateWorkingDatabase = "Could not create the working database";
    constexpr const char* CreateTable = "Could not create a table";
    constexpr const char* CreatePreparedStatement = "Could not create a prepared statement";
    constexpr const char* Transaction = "Could not start or end a transaction";
    constexpr const char* Insert = "Could not insert a row into the database";
    constexpr const char* Pragma = "Could not set a pragma";
    constexpr const char* Query = "Could not execute a query";
    constexpr const char* UnexpectedOutcome = "The program's operation encountered an unexpected outcome";
}


namespace ConcatConstants
{
    constexpr size_t MaxFilesPerBatch = 200;
    constexpr size_t MaxFileSizePerBatch = 100 * 1024 * 1024;

    constexpr int EventsPerTransaction = 1000;

    constexpr double AnalyzingEventsProgressPercent = 0.10;

    constexpr size_t WorkingAssociatedRowKeysMapMemorySize = 500 * 1024 * 1024;
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
        "SELECT `id`, `file_index` FROM `working_application_instance`  WHERE `file_index` >= ? ORDER BY `time` LIMIT 1 OFFSET ?;";

    // metadata
    constexpr const char* QueryColumnMetadata =
        "SELECT `metadata_column_info`.`column`, `metadata_column_info`.`type`, `metadata_column_info`.`nullable` "
        "FROM `metadata_column_info` "
        "JOIN `metadata_table_info` ON `metadata_column_info`.`metadata_table_info` = `metadata_table_info`.`id` "
        "WHERE `metadata_table_info`.`table` = ? "
        "ORDER BY `metadata_column_info`.`id`;";
}


struct Paradata::ConcatColumn
{
    Table::ColumnEntry column_entry;
    ConcatTable* associated_concat_table = nullptr;
};


struct Paradata::ConcatTable
{
    const TableDefinition& table_definition;
    std::vector<ConcatColumn> columns;
    bool auto_increment_id = false;
    bool auto_increment_if_unique_with_nulls = false;
    sqlite3_stmt* output_insert_stmt = nullptr;
    std::vector<sqlite3_stmt*> output_select_stmts;
    std::vector<sqlite3_stmt*> input_select_stmts;
};


Concatenator::Concatenator()
    :   m_outputDb(nullptr),
        m_outputDbIsCurrentlyOpenParadataLog(false),
        m_workingDb(nullptr),
        m_baseEventConcatTable(nullptr),
        m_startingFileIndex(0),
        m_currentFileIndex(0),
        m_operationProgressBarValue(0),
        m_totalProgressBarValue(0)
{
}


Concatenator::~Concatenator()
{
    Cleanup();
}


void Concatenator::Cleanup()
{
    // input databases
    CloseInputDatabases();

    // output database
    for( auto table_itr = m_concatTables.begin(); table_itr != m_concatTables.end(); ++table_itr )
    {
        ConcatTable* const concat_table = *table_itr;

        sqlite3_finalize(concat_table->output_insert_stmt);

        for( auto stmt_itr = concat_table->output_select_stmts.begin(); stmt_itr != concat_table->output_select_stmts.end(); ++stmt_itr )
            sqlite3_finalize(*stmt_itr);

        delete concat_table;
    }

    m_concatTables.clear();

    if( m_outputDbIsCurrentlyOpenParadataLog )
    {
        RestoreDatabasePragmas();
    }

    else if( m_outputDb != nullptr )
    {
        sqlite3_close(m_outputDb);
        m_outputDb = nullptr;
    }

    // working database
    if( m_workingDb != nullptr )
    {
        m_stmtInsertWorkingApplicationInstance.reset();
        m_workingAssociatedRowKeys.reset();

        sqlite3_close(m_workingDb);
        m_workingDb = nullptr;
    }
}


void Concatenator::CloseInputDatabases()
{
    for( auto table_itr = m_concatTables.begin(); table_itr != m_concatTables.end(); ++table_itr )
    {
        ConcatTable* const concat_table = *table_itr;

        for( auto stmt_itr = concat_table->input_select_stmts.begin(); stmt_itr != concat_table->input_select_stmts.end(); ++stmt_itr )
            sqlite3_finalize(*stmt_itr);

        concat_table->input_select_stmts.clear();
    }

    for( auto db_itr = m_inputDbs.begin(); db_itr != m_inputDbs.end(); ++db_itr)
        sqlite3_close(*db_itr);

    m_inputDbs.clear();
}


void Concatenator::OnInputProcessedSuccess(const std::variant<std::string, sqlite3*>& /*file_path_or_database*/, int64_t /*events_processed*/)
{
    // default implement does nothing
}


void Concatenator::OnInputProcessedError(const std::string& /*input_file_path*/, const char* const error_message)
{
    throw CSProException(error_message);
}


void Concatenator::OnProgressUpdate(const std::string& /*operation_message*/, int /*operation_percent*/, const char* /*total_message*/, int /*total_percent*/)
{
    // default implement does nothing
}


bool Concatenator::UserRequestsCancellation()
{
    return false;
}


template<typename T>
T Concatenator::ExecuteSingleQuery(sqlite3* const db, const cs::string_sz query, const std::optional<int> argument/* = std::nullopt*/)
{
    std::optional<SQLiteStatement> stmt;

    try
    {
        stmt.emplace(db, query.c_str(), true);
    }
    catch(...) { throw CSProException(ConcatErrors::CreatePreparedStatement); }

    if( argument.has_value() )
        stmt->Bind(1, *argument);

    if( stmt->Step() != SQLITE_ROW )
        throw CSProException(ConcatErrors::Query);

    return stmt->GetColumn<T>(0);
}


int64_t Concatenator::GetNumberEvents(sqlite3* const db)
{
    return ExecuteSingleQuery<int64_t>(db, ConcatSqlStatements::CountEvents);
}


void Concatenator::Run(const std::variant<std::string, sqlite3*>& output_file_path_or_database, const std::set<std::string>& paradata_log_file_paths)
{
    OnProgressUpdate(ConcatMessages::StartingOperation, 0, ConcatMessages::StartingOperation, 0);

    bool output_is_also_an_input = false;

    if( std::holds_alternative<std::string>(output_file_path_or_database) )
    {
        if( SO::IsBlank(std::get<std::string>(output_file_path_or_database)) )
            throw CSProException("You must specify the filename of the output log");
    }

    else
    {
        m_outputDbIsCurrentlyOpenParadataLog = true;

        m_outputDb = std::get<sqlite3*>(output_file_path_or_database);
        ASSERT(m_outputDb != nullptr);

        output_is_also_an_input = true;
    }

    // create batches of files to concatenate
    std::vector<std::vector<std::string>> input_file_paths;
    int64_t this_batch_file_size = 0;
    size_t number_inputs = 0;

    for( const std::string& paradata_log_file_path : paradata_log_file_paths )
    {
        const int64_t file_size = PortableFunctions::FileSize(paradata_log_file_path);

        if( file_size == -1 )
        {
            OnInputProcessedError(paradata_log_file_path, "Could not find the input log");
        }

        else if( std::holds_alternative<std::string>(output_file_path_or_database) &&
                 SO::EqualsNoCase(std::get<std::string>(output_file_path_or_database), paradata_log_file_path) )
        {
            output_is_also_an_input = true;
        }

        else
        {
            this_batch_file_size += file_size;

            if( input_file_paths.empty() ||
                this_batch_file_size >= ConcatConstants::MaxFileSizePerBatch ||
                input_file_paths.back().size() == ConcatConstants::MaxFilesPerBatch )
            {
                input_file_paths.emplace_back();
                this_batch_file_size = file_size;
            }

            ++number_inputs;
            input_file_paths.back().emplace_back(paradata_log_file_path);
        }
    }

    if( number_inputs == 0 )
        throw CSProException("You must specify at least one input log");


    // initialize the output file and working database
    if( !m_outputDbIsCurrentlyOpenParadataLog )
        CreateOutputFile(std::get<std::string>(output_file_path_or_database), output_is_also_an_input);

    SetupOutputConcatTables();

    if( output_is_also_an_input )
    {
        OnInputProcessedSuccess(output_file_path_or_database, GetNumberEvents(m_outputDb));
        SetDatabasePragmas(m_outputDb, false, true);
    }

    else // this will speed up the output
    {
        SetDatabasePragmas(m_outputDb, true, true);
    }

    CreateWorkingDatabase();

    // add the application instances from the output file (if it is also an input)
    if( output_is_also_an_input )
    {
        SetProgressUpdateInFile(output_file_path_or_database, ConcatMessages::AnalyzingEvents);
        AddApplicationInstancesToWorkingDatabase(m_outputDb);
    }


    // run the concatenation on each batch
    m_startingFileIndex = 1;
    const double total_progress_bar_file_step = CreatePercentMultiplier(number_inputs);

    for( const std::vector<std::string>& input_file_paths_batch : input_file_paths )
    {
        RunBatch(input_file_paths_batch, total_progress_bar_file_step);
        m_startingFileIndex += input_file_paths_batch.size();
    }

    // end the concatenation
    Cleanup();
}


void Concatenator::CreateOutputFile(const std::string& output_file_path, const bool output_is_also_an_input)
{
    if( !output_is_also_an_input && PortableFunctions::FileExists(output_file_path) && !PortableFunctions::FileDelete(output_file_path) )
        throw CSProException("Could not delete the output log");

    // opening the file via the Log class will create all the tables needed in the output file
    m_outputDb = Log::GetDatabaseForTool(output_file_path, true);
}


void Concatenator::CreateWorkingDatabase()
{
    // in release a temporary database will be created (in memory, and flushed to disk if necessary)
    const std::string working_db_file_path = DebugMode() ? GetUniqueTempFilePath("paraconcat.db") :
                                                           std::string();

    if( sqlite3_open(working_db_file_path.c_str(), &m_workingDb) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreateWorkingDatabase);

    SetDatabasePragmas(m_workingDb, true, true);

    if( sqlite3_exec(m_workingDb, ConcatSqlStatements::CreateWorkingApplicationInstanceTable, nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreateTable);

    try
    {
        m_stmtInsertWorkingApplicationInstance = std::make_unique<SQLiteStatement>(m_workingDb, ConcatSqlStatements::InsertWorkingApplicationInstance, true);
    }
    catch(...) { throw CSProException(ConcatErrors::CreatePreparedStatement); }

    // use roughly 500 mb of std::map memory before using SQLite for the associated rows keys,
    // and then only drop and rebuild the SQLite index for the second round of insertions (when it would affect 50% of rows)
    constexpr size_t map_size = ConcatConstants::WorkingAssociatedRowKeysMapMemorySize / ( sizeof(WorkingAssociatedRowKey) + sizeof(long) );
    m_workingAssociatedRowKeys = std::make_unique<ExpansiveMap<WorkingAssociatedRowKey, long>>(m_workingDb, map_size);
    m_workingAssociatedRowKeys->OptimizeForRecentInsertions(0.50);
}


void Concatenator::SetDatabasePragma(sqlite3* const db, const cs::string_sz pragma, const cs::string_sz value, const bool add_to_cache)
{
    // cache the current pragma setting when working with a paradata log that will continue to be used post-concatenation
    if( add_to_cache && m_outputDbIsCurrentlyOpenParadataLog )
    {
        const std::string query_pragma_sql = FormatText("PRAGMA %s;", pragma.c_str());
        m_pragmaSettingsToRestore.emplace_back(pragma.c_str(),
                                               ExecuteSingleQuery<std::string>(db, query_pragma_sql));
    }

    const std::string set_pragma_sql = FormatText("PRAGMA %s = %s;", pragma.c_str(), value.c_str());

    if( sqlite3_exec(db, set_pragma_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::Pragma);
}


void Concatenator::SetDatabasePragmas(sqlite3* const db, const bool set_journal_mode_off, const bool set_synchronous_off)
{
    constexpr const char* JournalMode     = "journal_mode";
    constexpr const char* SynchronousMode = "synchronous";

    if( set_journal_mode_off )
        SetDatabasePragma(db, JournalMode, "off", true);

    if( set_synchronous_off )
        SetDatabasePragma(db, SynchronousMode, "off", true);
}


void Concatenator::RestoreDatabasePragmas()
{
    ASSERT(m_outputDbIsCurrentlyOpenParadataLog && m_outputDb != nullptr);

    for( const auto& [pragma, value] : m_pragmaSettingsToRestore )
        SetDatabasePragma(m_outputDb, pragma, value, false);
}


void Concatenator::SetProgressUpdateInFile(const std::variant<std::string, sqlite3*>& output_file_path_or_database, const char* const total_message)
{
    const std::string progress_text = FormatText(ConcatMessages::WorkingInFile,
        std::holds_alternative<std::string>(output_file_path_or_database) ? PortableFunctions::PathGetFilename(std::get<std::string>(output_file_path_or_database)).c_str():
                                                                            "open paradata log");

    OnProgressUpdate(progress_text, static_cast<int>(m_operationProgressBarValue), total_message, static_cast<int>(m_totalProgressBarValue));
}


sqlite3* Concatenator::OpenInputFile(const std::string& input_file_path)
{
    return Log::GetDatabaseForTool(input_file_path, false);
}


void Concatenator::RunBatch(const std::vector<std::string>& input_file_paths, const double total_progress_bar_file_step)
{
    const double total_progress_bar_analyze_step = total_progress_bar_file_step * ConcatConstants::AnalyzingEventsProgressPercent;
    const double operation_progress_bar_analyze_step = CreatePercentMultiplier(input_file_paths.size());
    m_operationProgressBarValue = 0;

    // open the files and add the application instances
    for( m_currentFileIndex = 0; m_currentFileIndex < static_cast<int>(input_file_paths.size()); ++m_currentFileIndex )
    {
        const std::string& input_file_path = input_file_paths[m_currentFileIndex];

        SetProgressUpdateInFile(input_file_path, ConcatMessages::AnalyzingEvents);

        sqlite3* db;

        try
        {
            db = OpenInputFile(input_file_path);
            AddApplicationInstancesToWorkingDatabase(db);
        }

        catch( const CSProException& exception )
        {
            OnInputProcessedError(input_file_path, exception.what());
            db = nullptr;
        }

        m_inputDbs.emplace_back(db);

        if( UserRequestsCancellation() )
            throw UserCanceledException();

        m_operationProgressBarValue += operation_progress_bar_analyze_step;
        m_totalProgressBarValue += total_progress_bar_analyze_step;
    }


    // run through the (non-duplicative) application instances, in timestamp order, and
    // insert events from those application instances in the output file
    const size_t number_application_interfaces = ExecuteSingleQuery<size_t>(m_workingDb, ConcatSqlStatements::CountWorkingApplicationInstances, m_startingFileIndex);
    std::vector<int64_t> number_application_events_per_file(m_inputDbs.size(), 0);

    if( number_application_interfaces > 0 )
    {
        const double total_progress_bar_concatenating_step = ( total_progress_bar_file_step - total_progress_bar_analyze_step ) * input_file_paths.size() / number_application_interfaces;
        const double operation_progress_bar_concatenating_step = CreatePercentMultiplier(number_application_interfaces);
        m_operationProgressBarValue = 0;

        std::optional<SQLiteStatement> stmt;

        try
        {
            stmt.emplace(m_workingDb, ConcatSqlStatements::QueryWorkingApplicationInstances, true);
        }
        catch(...) { throw CSProException(ConcatErrors::CreatePreparedStatement); }

        for( size_t offset = 0; offset < number_application_interfaces; ++offset )
        {
            long application_instance_id;

            // because we cannot lock the database in case the index is dropped and rebuild (for m_workingAssociatedRowKeys),
            // we use an offset to to make sure that the statement is always reset to avoid a SQLITE_LOCKED error
            {
                const SQLiteResetOnDestruction rod(*stmt);

                stmt->Bind(1, m_startingFileIndex);
                stmt->Bind(2, offset);

                if( stmt->Step() != SQLITE_ROW )
                    throw CSProException(ConcatErrors::UnexpectedOutcome);

                application_instance_id = stmt->GetColumn<long>(0);
                m_currentFileIndex = stmt->GetColumn<int>(1) - m_startingFileIndex;
            }

            if( m_inputDbs[m_currentFileIndex] != nullptr )
            {
                SetProgressUpdateInFile(input_file_paths[m_currentFileIndex], ConcatMessages::ConcatenatingEvents);

                number_application_events_per_file[m_currentFileIndex] += ConcatenateEvents(application_instance_id);

                if( UserRequestsCancellation() )
                    throw UserCanceledException();
            }

            m_operationProgressBarValue += operation_progress_bar_concatenating_step;
            m_totalProgressBarValue += total_progress_bar_concatenating_step;
        }
    }


    // wrap up work on this batch
    for( size_t i = 0; i < m_inputDbs.size(); ++i )
    {
        if( m_inputDbs[i] != nullptr )
            OnInputProcessedSuccess(input_file_paths[i], number_application_events_per_file[i]);
    }

    CloseInputDatabases();

    // the associated rows keys for these logs will no longer be used
    m_workingAssociatedRowKeys->Clear();
}


void Concatenator::AddApplicationInstancesToWorkingDatabase(sqlite3* const db)
{
    // query the application instances
    sqlite3_stmt* stmt;

    if( sqlite3_prepare_v2(db, ConcatSqlStatements::QueryApplicationInstancesFromLog, -1, &stmt, nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreatePreparedStatement);

    while( sqlite3_step(stmt) == SQLITE_ROW )
    {
        const std::string uuid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const long application_instance_id = static_cast<long>(sqlite3_column_int64(stmt, 1));
        const double timestamp = sqlite3_column_double(stmt, 2);

        // update the working database
        const SQLiteResetOnDestruction rod(*m_stmtInsertWorkingApplicationInstance);

        m_stmtInsertWorkingApplicationInstance->Bind(1, uuid)
                                               .Bind(2, application_instance_id)
                                               .Bind(3, timestamp)
                                               .Bind(4, m_currentFileIndex + m_startingFileIndex);

        if( m_stmtInsertWorkingApplicationInstance->Step() != SQLITE_DONE )
            throw CSProException(ConcatErrors::Insert);
    }

    sqlite3_finalize(stmt);
}


void Concatenator::SetupOutputConcatTables()
{
    // first add all of the tables
    for( size_t i = ParadataTable_FirstNonMetadataTableIndex; i < ParadataTable_NumberTables; ++i )
    {
        ConcatTable* const concat_table = m_concatTables.emplace_back(new ConcatTable { GetTableDefinition(static_cast<ParadataTable>(i)) });

        if( concat_table->table_definition.table_code > 0 ) // an event
        {
            m_eventTypeToConcatTableMap[concat_table->table_definition.table_code] = concat_table;
        }

        else if( concat_table->table_definition.type == ParadataTable::BaseEvent )
        {
            m_baseEventConcatTable = concat_table;
        }
    }

    // then read all of the column information from the database
    sqlite3_stmt* stmt;

    if( sqlite3_prepare_v2(m_outputDb, ConcatSqlStatements::QueryColumnMetadata, -1, &stmt, nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::CreatePreparedStatement);

    for( auto table_itr = m_concatTables.begin(); table_itr != m_concatTables.end(); ++table_itr )
    {
        ConcatTable* const concat_table = *table_itr;

        sqlite3_reset(stmt);

        sqlite3_bind_text(stmt, 1, concat_table->table_definition.name, -1, SQLITE_TRANSIENT);

        while( sqlite3_step(stmt) == SQLITE_ROW )
        {
            ConcatColumn& concat_column = concat_table->columns.emplace_back();
            concat_column.column_entry.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            concat_column.column_entry.type = Table::StringToColumnType(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            concat_column.column_entry.nullable = ( sqlite3_column_int(stmt, 2) == 1 );

            // check if this column's value points to another table
            if( concat_column.column_entry.type == Table::ColumnType::Long )
            {
                for( auto second_table_itr = m_concatTables.begin(); second_table_itr != m_concatTables.end(); ++second_table_itr )
                {
                    const size_t table_name_pos = concat_column.column_entry.name.find((*second_table_itr)->table_definition.name);

                    // if the table name is at the end, it's a match
                    if( ( table_name_pos != std::string::npos ) &&
                        ( ( table_name_pos + strlen((*second_table_itr)->table_definition.name) ) == concat_column.column_entry.name.length() ) )
                    {
                        concat_column.associated_concat_table = *second_table_itr;
                        break;
                    }
                }
            }

            concat_table->auto_increment_id = ( concat_table->table_definition.insert_type == InsertType::AutoIncrement ||
                                                concat_table->table_definition.insert_type == InsertType::AutoIncrementIfUnique );

            if( concat_column.column_entry.nullable && concat_table->table_definition.insert_type == InsertType::AutoIncrementIfUnique )
                concat_table->auto_increment_if_unique_with_nulls = true;
        }

        if( concat_table->columns.empty() )
            throw CSProException(ConcatErrors::UnexpectedOutcome);
    }

    sqlite3_finalize(stmt);
}


sqlite3_stmt* Concatenator::GetInputSelectStatement(ConcatTable& concat_table, const char* const where_column_name)
{
    // see if the statement already has been created
    if( concat_table.input_select_stmts.size() <= static_cast<size_t>(m_currentFileIndex) )
        concat_table.input_select_stmts.resize(m_currentFileIndex + 1, nullptr);

    sqlite3_stmt*& stmt = concat_table.input_select_stmts[m_currentFileIndex];

    // if so, reset it and use it
    if( stmt != nullptr )
    {
        sqlite3_reset(stmt);
    }

    // if not, create it
    else
    {
        std::string select_sql = "SELECT `id`";

        for( const ConcatColumn& concat_column : concat_table.columns )
            select_sql.append(FormatText(", `%s`", concat_column.column_entry.name.c_str()));

        select_sql.append(FormatText(" FROM `%s` WHERE `%s` = ? ORDER BY `id`;", concat_table.table_definition.name, where_column_name));

        if( sqlite3_prepare_v2(m_inputDbs[m_currentFileIndex], select_sql.c_str(), select_sql.length(), &stmt, nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreatePreparedStatement);
    }

    return stmt;
}


sqlite3_stmt* Concatenator::GetOutputSelectStatement(ConcatTable& concat_table, const int null_values_flag)
{
    // see if the statement already has been created
    if( concat_table.output_select_stmts.size() <= static_cast<size_t>(null_values_flag) )
        concat_table.output_select_stmts.resize(null_values_flag + 1, nullptr);

    sqlite3_stmt*& stmt = concat_table.output_select_stmts[null_values_flag];

    // if so, reset it and use it
    if( stmt != nullptr )
    {
        sqlite3_reset(stmt);
    }

    // if not, create it
    else
    {
        std::string select_sql = FormatText("SELECT `id` FROM `%s` WHERE", concat_table.table_definition.name);

        for( size_t i = 0; i < concat_table.columns.size(); ++i )
        {
            const bool is_null = ( ( null_values_flag & ( 1 << i ) ) != 0 );

            select_sql.append(FormatText(" %s`%s` %s", ( i == 0 ) ? "" : "AND ",
                                         concat_table.columns[i].column_entry.name.c_str(),
                                         is_null ? "is null" : "= ?"));
        }

        select_sql.append(" ORDER BY `id`;");

        if( sqlite3_prepare_v2(m_outputDb, select_sql.c_str(), select_sql.length(), &stmt, nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreatePreparedStatement);
    }

    return stmt;
}


sqlite3_stmt* Concatenator::GetOutputInsertStatement(ConcatTable& concat_table)
{
    if( concat_table.output_insert_stmt != nullptr )
    {
        sqlite3_reset(concat_table.output_insert_stmt);
    }

    else // create it
    {
        std::string insert_sql = FormatText("INSERT INTO `%s` ( %s", concat_table.table_definition.name,
                                                                     concat_table.auto_increment_id ? "" : "`id`");

        std::string insert_value_sql = concat_table.auto_increment_id ? "" : "?";

        for( size_t i = 0; i < concat_table.columns.size(); ++i )
        {
            if( !concat_table.auto_increment_id || i != 0 )
            {
                insert_sql.push_back(',');
                insert_value_sql.push_back(',');
            }

            insert_sql.append(FormatText("`%s`", concat_table.columns[i].column_entry.name.c_str()));
            insert_value_sql.push_back('?');
        }

        insert_sql.append(FormatText(" ) VALUES ( %s );", insert_value_sql.c_str()));

        if( sqlite3_prepare_v2(m_outputDb, insert_sql.c_str(), insert_sql.length(), &concat_table.output_insert_stmt, nullptr) != SQLITE_OK )
            throw CSProException(ConcatErrors::CreatePreparedStatement);
    }

    return concat_table.output_insert_stmt;
}


void Concatenator::ManageTransaction(const bool begin)
{
    const char* const sql = begin ? Sqlite::Commands::BeginTransaction :
                                    Sqlite::Commands::EndTransaction;

    if( sqlite3_exec(m_outputDb, sql, nullptr, nullptr, nullptr) != SQLITE_OK )
        throw CSProException(ConcatErrors::Transaction);
}


int64_t Concatenator::ConcatenateEvents(const long application_instance_id)
{
    int64_t events = 0;

    ManageTransaction(true);
    int events_until_next_transaction = ConcatConstants::EventsPerTransaction;

    sqlite3_stmt* const stmt_select_base_event = GetInputSelectStatement(*m_baseEventConcatTable, "application_instance");
    sqlite3_bind_int64(stmt_select_base_event, 1, application_instance_id);

    while( sqlite3_step(stmt_select_base_event) == SQLITE_ROW )
    {
        const long input_base_event_id = static_cast<long>(sqlite3_column_int64(stmt_select_base_event, 0));
        const int event_type = sqlite3_column_int(stmt_select_base_event, 4);

        // process the base event
        long output_base_event_id = 0;
        ConcatenateRow(&output_base_event_id, *m_baseEventConcatTable, stmt_select_base_event);

        // process the related event
        const auto& this_event_concat_table_lookup = m_eventTypeToConcatTableMap.find(event_type);

        if( this_event_concat_table_lookup == m_eventTypeToConcatTableMap.end() )
            throw CSProException(ConcatErrors::UnexpectedOutcome);

        sqlite3_stmt* const stmt_select_related_event = GetInputSelectStatement(*this_event_concat_table_lookup->second, "id");
        sqlite3_bind_int64(stmt_select_related_event, 1, input_base_event_id);

        if( sqlite3_step(stmt_select_related_event) != SQLITE_ROW )
            throw CSProException(ConcatErrors::UnexpectedOutcome);

        ConcatenateRow(&output_base_event_id, *this_event_concat_table_lookup->second, stmt_select_related_event);

        ++events;

        if( --events_until_next_transaction == 0 )
        {
            ManageTransaction(false);

            if( UserRequestsCancellation() )
                throw UserCanceledException();

            ManageTransaction(true);
            events_until_next_transaction = ConcatConstants::EventsPerTransaction;
        }
    }

    ManageTransaction(false);

    return events;
}


void Concatenator::BindArguments(ConcatTable& concat_table, sqlite3_stmt* const stmt_input, sqlite3_stmt* const stmt_output,
                                 int output_bind_index, std::set<ConcatTable*>* const caller_queried_tables/* = nullptr*/)
{
    int input_reference_index = 1; // to skip over the ID column

    for( auto itr = concat_table.columns.begin(); itr != concat_table.columns.end(); ++itr )
    {
        if( itr->column_entry.nullable && sqlite3_column_type(stmt_input, input_reference_index) == SQLITE_NULL )
        {
            sqlite3_bind_null(stmt_output, output_bind_index);
        }

        else
        {
            switch( itr->column_entry.type )
            {
                case Table::ColumnType::Boolean:
                case Table::ColumnType::Integer:
                    sqlite3_bind_int(stmt_output, output_bind_index, sqlite3_column_int(stmt_input, input_reference_index));
                    break;

                case Table::ColumnType::Long:
                {
                    long value = static_cast<long>(sqlite3_column_int64(stmt_input, input_reference_index));
                    bool bind_long = true;

                    if( itr->associated_concat_table != nullptr )
                    {
                        if( value > 0 )
                        {
                            value = ConcatenateAssociatedRow(value, *itr->associated_concat_table, caller_queried_tables);
                        }

                        // if using an old paradata log, the link may not be defined
                        else
                        {
                            bind_long = false;
                        }
                    }

                    if( bind_long )
                    {
                        sqlite3_bind_int64(stmt_output, output_bind_index, value);
                    }

                    else
                    {
                        sqlite3_bind_null(stmt_output, output_bind_index);
                    }

                    break;
                }

                case Table::ColumnType::Double:
                    sqlite3_bind_double(stmt_output, output_bind_index, sqlite3_column_double(stmt_input, input_reference_index));
                    break;

                default: // ColumnType::Text
                    sqlite3_bind_text(stmt_output,output_bind_index, reinterpret_cast<const char*>(sqlite3_column_text(stmt_input, input_reference_index)), -1, SQLITE_TRANSIENT);
                    break;
            }
        }

        ++input_reference_index;
        ++output_bind_index;
    }
}


void Concatenator::ConcatenateRow(long* const id, ConcatTable& concat_table, sqlite3_stmt* const stmt_input)
{
    sqlite3_stmt* const stmt_output = GetOutputInsertStatement(concat_table);

    // bind the arguments
    int output_bind_index = 1;

    // bind the ID if necessary
    if( !concat_table.auto_increment_id )
        sqlite3_bind_int64(stmt_output, output_bind_index++, *id);

    BindArguments(concat_table, stmt_input, stmt_output, output_bind_index);

    // insert the row
    if( sqlite3_step(stmt_output) != SQLITE_DONE )
        throw CSProException(ConcatErrors::Insert);

    if( concat_table.auto_increment_id )
        *id = static_cast<long>(sqlite3_last_insert_rowid(m_outputDb));
}


long Concatenator::ConcatenateAssociatedRow(const long input_id, ConcatTable& concat_table, std::set<ConcatTable*>* const caller_queried_tables/* = nullptr*/)
{
    // check if this has already been added to the output
    WorkingAssociatedRowKey row_key(m_currentFileIndex + m_startingFileIndex,
                                    static_cast<int>(concat_table.table_definition.type),
                                    input_id);

    const cs::shared_or_raw_ptr<const long> row_id = m_workingAssociatedRowKeys->Find(row_key);

    if( row_id != nullptr )
        return *row_id;

    // if not, check if it already exists (for AutoIncrementIfUnique tables) or insert it
    sqlite3_stmt* const stmt_input = GetInputSelectStatement(concat_table, "id");

    if( caller_queried_tables != nullptr )
        caller_queried_tables->insert(&concat_table);

    // this code is in a loop because of the recursive nature of this function, which can
    // lead to the prepared statement being messed up (for example, if name.parent_name
    // needs to be added, then the query on the initial name would be messed up); there
    // would be ways to do this more efficiently, but this approach works and isn't too
    // inefficient because after a short amount of time all the queries will return above
    long output_id = 0;
    bool must_insert_row = true;
    bool keep_processing = true;

    while( keep_processing )
    {
        std::set<ConcatTable*> these_caller_queried_tables;

        sqlite3_reset(stmt_input);
        sqlite3_bind_int64(stmt_input, 1, input_id);

        if( sqlite3_step(stmt_input) != SQLITE_ROW )
            throw CSProException(ConcatErrors::UnexpectedOutcome);

        // check if it already exists
        if( concat_table.table_definition.insert_type == InsertType::AutoIncrementIfUnique )
        {
            int null_values_flag = 0;

            if( concat_table.auto_increment_if_unique_with_nulls )
            {
                for( size_t i = 0; i < concat_table.columns.size(); ++i )
                {
                    // + 1 to skip over the ID column
                    if( sqlite3_column_type(stmt_input, i + 1) == SQLITE_NULL  )
                        null_values_flag |= ( 1 << i );
                }
            }

            sqlite3_stmt* const stmt_exists_query = GetOutputSelectStatement(concat_table, null_values_flag);

            BindArguments(concat_table, stmt_input, stmt_exists_query, 1, &these_caller_queried_tables);

            if( caller_queried_tables != nullptr )
                caller_queried_tables->insert(these_caller_queried_tables.begin(), these_caller_queried_tables.end());

            // if the bound arguments are still valid, we can get out of the loop
            if( these_caller_queried_tables.find(&concat_table) == these_caller_queried_tables.end() )
            {
                if( sqlite3_step(stmt_exists_query) == SQLITE_ROW )
                {
                    output_id = static_cast<long>(sqlite3_column_int64(stmt_exists_query,0));
                    must_insert_row = false;
                }

                keep_processing = false;
            }
        }

        else
        {
            keep_processing = false;
        }
    }

    // it does not exist, so insert it
    if( must_insert_row )
        ConcatenateRow(&output_id, concat_table, stmt_input);

    // and add the reference to the working database
    try
    {
        m_workingAssociatedRowKeys->Insert(std::move(row_key), output_id);
    }
    catch(...) { throw CSProException(ConcatErrors::Insert); }

    return output_id;
}


// --------------------------------------------------------------------------
// ExpansiveMap_KeyBinder specialization for WorkingAssociatedRowKey
// --------------------------------------------------------------------------

template<>
constexpr int ExpansiveMap_KeyBinder<std::tuple<int, int, long>>::GetNumberKeys()
{
    return 3;
}


template<>
std::vector<const char*> ExpansiveMap_KeyBinder<std::tuple<int, int, long>>::GetDataTypes()
{
    return { Sqlite::GetDataType<int>(),
             Sqlite::GetDataType<int>(),
             Sqlite::GetDataType<long>() };
}


template<>
template<typename KeyT>
void ExpansiveMap_KeyBinder<std::tuple<int, int, long>>::Bind(SQLiteStatement& stmt, KeyT&& key)
{
    stmt.Bind(1, std::get<0>(key))
        .Bind(2, std::get<1>(key))
        .Bind(3, std::get<2>(key));
}
