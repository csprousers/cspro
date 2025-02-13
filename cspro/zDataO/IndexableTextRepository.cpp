#include "stdafx.h"
#include "IndexableTextRepository.h"
#include <SQLite/SQLiteHelpers.h>
#include <zUtilF/ThreadedProgressDlg.h>


namespace Constants
{
    constexpr int64_t MinFileSizeForIndexProgressDialog = 128 * 1024;
    constexpr size_t IndexProgressUpdateFrequency       = 128;
}


namespace SqlStatements
{
    constexpr const char* SetJournalModeToOff = "PRAGMA journal_mode = OFF;";
    constexpr const char* SetJournalModeToDelete = "PRAGMA journal_mode = DELETE;";
    constexpr const char* SetSynchronousOff = "PRAGMA synchronous = OFF;";
    constexpr const char* SetSynchronousFull = "PRAGMA synchronous = FULL;";

    constexpr const char* CreateIdStructureTable = "CREATE TABLE `id-structure` (`name` TEXT PRIMARY KEY UNIQUE NOT NULL, `structure` INTEGER NOT NULL);";
    constexpr const char* QueryIdStructure = "SELECT `structure` FROM `id-structure` WHERE `name` = ? LIMIT 1;";
    constexpr const char* InsertIdStructure = "INSERT INTO `id-structure` (`name`, `structure`) VALUES ( ?, ? );";
}


IndexableTextRepository::IndexableTextRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type)
    :   DataRepository(type, std::move(case_access), access_type),
        m_requiresIndex(( m_accessType == DataRepositoryAccess::ReadOnly ) ||
                        ( m_accessType == DataRepositoryAccess::ReadWrite ) ||
                        ( m_accessType == DataRepositoryAccess::EntryInput )),
        m_db(nullptr),
        m_triedCreatingNonRequiredIndex(false),
        m_indexerCallback(nullptr)
{
}


IndexableTextRepository::~IndexableTextRepository()
{
    try
    {
        if( m_db != nullptr )
            CloseIndex();
    }

    catch( const DataRepositoryException::Error& )
    {
        // ignore errors
    }
}


void IndexableTextRepository::CloseIndex()
{
    if( m_db == nullptr )
        return;

    // finalize any prepared statements
    for( std::shared_ptr<SQLiteStatement>* stmt : m_preparedStatements )
        stmt->reset();

    m_preparedStatements.clear();

    // close the database
    if( sqlite3_close(m_db) != SQLITE_OK )
        throw DataRepositoryException::SQLiteError();

    m_db = nullptr;

    // if the data file was updated via only WriteMethod::Replace, then the data file will have been updated
    // later than the index, which would cause the index to be regenerated on the next load; this will force the
    // modify date of the index to be greater than the data file
    if( PortableFunctions::FileModifiedTime(m_connectionString.GetFilename()) > PortableFunctions::FileModifiedTime(m_indexFilename) )
        PortableFunctions::FileTouch(m_indexFilename);
}


void IndexableTextRepository::SetIndexFilename()
{
    m_indexFilename = WS2CS(m_connectionString.GetFilename() + FileExtensions::Data::WithDot::IndexableTextIndex);
}


void IndexableTextRepository::CreateOrOpenIndex(bool create_new_data_file)
{
    SetIndexFilename();

    if( !create_new_data_file && IsIndexValid() )
    {
        CreatePreparedStatements();
    }

    else
    {
        auto create_index_failure_tasks = [&]
        {
            CloseIndex();
            PortableFunctions::FileDelete(m_indexFilename);
        };

        try
        {
            CreateIndex();
        }

        catch( const UserCanceledException& )
        {
            create_index_failure_tasks();

            if( m_indexerCallback != nullptr )
            {
                throw;
            }

            else
            {
                throw DataRepositoryException::IOError(MGF::GetMessageText(MGF::IndexCannotCreate));
            }
        }

        catch( const DataRepositoryException::Error& )
        {
            create_index_failure_tasks();
            throw;
        }
    }
}


bool IndexableTextRepository::IsIndexValid()
{
    ASSERT(!m_indexFilename.IsEmpty());

    // if called from CSIndex, always say the index is false so that it will be recreated
    if( m_indexerCallback != nullptr )
        return false;

    // an index is valid if:
    // - the index exists;
    // - the data file is older than the index; and
    // - the structure of the IDs has not changed
    if( !PortableFunctions::FileIsRegular(m_indexFilename) ||
        PortableFunctions::FileModifiedTime(m_connectionString.GetFilename()) > PortableFunctions::FileModifiedTime(m_indexFilename) )
    {
        return false;
    }

    bool index_is_valid = false;

    // open the existing index and make sure that the current dictionary matches the one used to generate the index
    if( sqlite3_open(ToUtf8(m_indexFilename), &m_db) == SQLITE_OK )
    {
        try
        {
            SQLiteStatement stmt_query_id_structure(m_db, SqlStatements::QueryIdStructure, true);

            stmt_query_id_structure.Bind(1, m_caseAccess->GetDataDict().GetName());

            stmt_query_id_structure.StepCheckResult(SQLITE_ROW);

            // keep the index open if it was valid
            if( stmt_query_id_structure.GetColumn<size_t>(0) == GetIdStructureHashForKeyIndex() )
                return true;
        }

        catch( const SQLiteStatementException& )
        {
            ASSERT(!index_is_valid);
        }

        // close the database if we need to create a new index
        CloseIndex();
    }

    return false;
}


void IndexableTextRepository::CreateIndex()
{
    ASSERT(!m_indexFilename.IsEmpty());

    auto do_sqlite3_exec = [&](const char* sql)
    {
        if( sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr) != SQLITE_OK )
            throw DataRepositoryException::SQLiteError();
    };

    // delete the existing index if necessary
    if( PortableFunctions::FileIsRegular(m_indexFilename) && !PortableFunctions::FileDelete(m_indexFilename) )
        throw DataRepositoryException::IOError(_T("The index file, which needs to be regenerated, could not be deleted."));

    // open the database
    if( sqlite3_open(ToUtf8(m_indexFilename), &m_db) != SQLITE_OK )
        throw DataRepositoryException::SQLiteError();

    // speed up the creation of the index because any errors on creation can be overcome
    // by deleting the file and recreating it
    do_sqlite3_exec(SqlStatements::SetJournalModeToOff);
    do_sqlite3_exec(SqlStatements::SetSynchronousOff);

    // create the tables and fill in the key descriptions
    do_sqlite3_exec(SqlStatements::BeginTransaction);


    // create the ID structure table
    do_sqlite3_exec(SqlStatements::CreateIdStructureTable);

    // insert information about the ID structure
    try
    {
        SQLiteStatement stmt_insert_id_structure(m_db, SqlStatements::InsertIdStructure);
        stmt_insert_id_structure.Bind(1, m_caseAccess->GetDataDict().GetName());
        stmt_insert_id_structure.Bind(2, GetIdStructureHashForKeyIndex());
        stmt_insert_id_structure.StepCheckResult(SQLITE_DONE);
    }

    catch( const SQLiteStatementException& )
    {
        throw DataRepositoryException::SQLiteError();
    }


    // initialize the index creator
    std::shared_ptr<IndexCreator> index_creator = GetIndexCreator();
    ASSERT(index_creator != nullptr);

    bool using_indexer_callback = ( m_indexerCallback != nullptr );
    index_creator->Initialize(!using_indexer_callback);


    // create the keys table
    do_sqlite3_exec(index_creator->GetCreateKeyTableSql());

    do_sqlite3_exec(SqlStatements::EndTransaction);

    CreatePreparedStatements();


    // now generate the index, potentially showing a progress bar
    std::unique_ptr<ThreadedProgressDlg> progress_dlg;
    size_t cases_until_progress_update = Constants::IndexProgressUpdateFrequency;

    if( using_indexer_callback || index_creator->GetFileSize() >= Constants::MinFileSizeForIndexProgressDialog )
    {
        progress_dlg = std::make_unique<ThreadedProgressDlg>();
        progress_dlg->SetTitle(_T("Creating Index ..."));
        progress_dlg->SetStatus(FormatText(_T("Creating an index for %s"), PortableFunctions::PathGetFilename(m_connectionString.GetFilename())));
        progress_dlg->Show();
    }


    // wrap inserts in a transaction
    size_t cases_until_transaction_commit;

    auto begin_transaction = [&]()
    {
        cases_until_transaction_commit = MaxNumberSqlInsertsInOneTransaction;
        do_sqlite3_exec(SqlStatements::BeginTransaction);
    };

    begin_transaction();
        

    // read each case
    IndexableTextRepositoryIndexDetails index_details;
    bool index_cannot_be_created = false;

    while( index_creator->ReadCaseAndUpdateIndex(index_details) )
    {
        // call back into CSIndex (if appropriate)
        if( using_indexer_callback )
            m_indexerCallback->IndexCallback(index_details);


        if( index_details.case_prevents_index_creation )
        {
            index_cannot_be_created = true;
        }

        else
        {
            // end the transaction if the number of inserts reaches the max for a transaction
            if( --cases_until_transaction_commit == 0 )
            {
                do_sqlite3_exec(SqlStatements::EndTransaction);

                // start a new transaction
                begin_transaction();
            }
        }


        // update the progress bar
        if( progress_dlg != nullptr )
        {
            if( progress_dlg->IsCanceled() )
                throw UserCanceledException();

            if( --cases_until_progress_update == 0 )
            {
                progress_dlg->SetPos(index_creator->GetPercentRead());
                cases_until_progress_update = Constants::IndexProgressUpdateFrequency;
            }
        }
    }


    // when running via CSIndex, throw an exception if the index cannot be successfully created
    if( index_cannot_be_created )
    {
        ASSERT(using_indexer_callback);
        throw DataRepositoryException::DuplicateCaseWhileCreatingIndex(CString());
    }


    // create any indices
    for( const char* create_index_sql : index_creator->GetCreateIndexSqlStatements() )
        do_sqlite3_exec(create_index_sql);
    

    // end the last transaction and restore the default journal_mode and synchronous settings
    do_sqlite3_exec(SqlStatements::EndTransaction);

    do_sqlite3_exec(SqlStatements::SetSynchronousFull);
    do_sqlite3_exec(SqlStatements::SetJournalModeToDelete);


    // notify the actual repository that the index was created successfully
    index_creator->OnSuccessfulCreation();
}


void IndexableTextRepository::OpenInitiallyNonRequiredIndex() const
{
    ASSERT(m_db == nullptr && m_accessType == DataRepositoryAccess::BatchInput);

    if( !m_triedCreatingNonRequiredIndex )
    {
        IndexableTextRepository& non_const_repository = const_cast<IndexableTextRepository&>(*this);
        non_const_repository.m_triedCreatingNonRequiredIndex = true;
        non_const_repository.SetIndexFilename();

        bool index_is_valid = non_const_repository.IsIndexValid();

        if( !index_is_valid )
        {
            // try to open the file as read only, which will force the creation of an index
            std::unique_ptr<DataRepository> read_only_repository = DataRepository::CreateAndOpen(m_caseAccess,
                                                                                                 m_connectionString,
                                                                                                 DataRepositoryAccess::ReadOnly,
                                                                                                 DataRepositoryOpenFlag::OpenMustExist);
            read_only_repository->Close();

            index_is_valid = non_const_repository.IsIndexValid();
        }

        if( index_is_valid )
        {
            non_const_repository.CreatePreparedStatements();
            non_const_repository.OpenBatchInputDataFileAsIndexed();
        }
    }

    if( m_db == nullptr )
        throw DataRepositoryException::IndexRequired();
}


void IndexableTextRepository::CreatePreparedStatements()
{
    if( !m_preparedStatements.empty() )
    {
        ASSERT(false);
        return;
    }

    // some prepared statements will only be created when needed, but the most common ones are created here
    for( const auto& [sql, stmt] : GetSqlStatementsToPrepare() )
        PrepareSqlStatementForQuery(sql, stmt);
}


void IndexableTextRepository::PrepareSqlStatementForQuery(SqlQueryType type, std::shared_ptr<SQLiteStatement>& stmt)
{
    ASSERT(stmt == nullptr);

    std::variant<const char*, std::shared_ptr<SQLiteStatement>> sql_or_stmt = GetSqlStatementForQuery(type);

    if( std::holds_alternative<const char*>(sql_or_stmt) )
    {
        const char* sql = std::get<const char*>(sql_or_stmt);
        PrepareSqlStatementForQuery(sql, stmt);
    }

    else
    {
        stmt = std::move(std::get<1>(sql_or_stmt));

        ASSERT(std::find(m_preparedStatements.cbegin(), m_preparedStatements.cend(), &stmt) == m_preparedStatements.cend());
        ASSERT(std::find_if(m_preparedStatements.cbegin(), m_preparedStatements.cend(),
                            [&](const auto& this_stmt) { return ( *this_stmt == stmt ); }) != m_preparedStatements.cend());

        m_preparedStatements.emplace_back(&stmt);
    }

    ASSERT(std::find(m_preparedStatements.cbegin(), m_preparedStatements.cend(), &stmt) != m_preparedStatements.cend());    

    ASSERT(stmt != nullptr);
}


void IndexableTextRepository::PrepareSqlStatementForQuery(const char* sql, std::shared_ptr<SQLiteStatement>& stmt)
{
    ASSERT(stmt == nullptr);

    EnsureIndexIsOpen();

    try
    {
        stmt = std::make_shared<SQLiteStatement>(m_db, sql, true);
        m_preparedStatements.emplace_back(&stmt);
    }

    catch( const SQLiteStatementException& )
    {
        throw DataRepositoryException::SQLiteError();
    }    
}


SQLiteStatement IndexableTextRepository::PrepareSqlStatementForQuery(wstring_view sql) const
{
    EnsureIndexIsOpen();

    try
    {
        return SQLiteStatement(m_db, sql, true);
    }

    catch( const SQLiteStatementException& )
    {
        throw DataRepositoryException::SQLiteError();
    }    
}


std::tuple<int64_t, size_t> IndexableTextRepository::GetPositionBytesFromKey(const CString& key, bool throw_exception/* = true*/) const
{
    EnsureSqlStatementIsPrepared(SqlQueryType::GetPositionBytesFromNotDeletedKey, m_stmtQueryPositionBytesByNotDeletedKey);
    SQLiteResetOnDestruction rod(*m_stmtQueryPositionBytesByNotDeletedKey);

    m_stmtQueryPositionBytesByNotDeletedKey->Bind(1, key);

    if( m_stmtQueryPositionBytesByNotDeletedKey->Step() == SQLITE_ROW )
    {
        return { m_stmtQueryPositionBytesByNotDeletedKey->GetColumn<int64_t>(0),
                 m_stmtQueryPositionBytesByNotDeletedKey->GetColumn<size_t>(1) };
    }

    else if( throw_exception )
    {
        throw DataRepositoryException::CaseNotFound();
    }

    else
    {
        return { -1, 0 };
    }
}


size_t IndexableTextRepository::GetBytesFromPosition(int64_t file_position) const
{
    EnsureSqlStatementIsPrepared(SqlQueryType::GetBytesFromPosition, m_stmtQueryBytesByPosition);
    SQLiteResetOnDestruction rod(*m_stmtQueryBytesByPosition);

    m_stmtQueryBytesByPosition->Bind(1, file_position);

    if( m_stmtQueryBytesByPosition->Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    return m_stmtQueryBytesByPosition->GetColumn<size_t>(0);
}


bool IndexableTextRepository::ContainsCase(const CString& key) const
{
    EnsureSqlStatementIsPrepared(SqlQueryType::ContainsNotDeletedKey, m_stmtNotDeletedKeyExists);
    SQLiteResetOnDestruction rod(*m_stmtNotDeletedKeyExists);

    m_stmtNotDeletedKeyExists->Bind(1, key);

    return ( m_stmtNotDeletedKeyExists->Step() == SQLITE_ROW );
}


void IndexableTextRepository::ReadCase(Case& data_case, const CString& key)
{
    auto [file_position, bytes_for_case] = GetPositionBytesFromKey(key);

    ReadCase(data_case, file_position, bytes_for_case);

    ASSERT(data_case.GetKey() == key && !data_case.GetDeleted() && data_case.GetPositionInRepository() >= 0);
}


void IndexableTextRepository::ReadCase(Case& data_case, double position_in_repository)
{
    int64_t file_position = static_cast<int64_t>(position_in_repository);
    size_t bytes_for_case = GetBytesFromPosition(file_position);

    ReadCase(data_case, file_position, bytes_for_case);

    ASSERT(data_case.GetPositionInRepository() == file_position);
}


void IndexableTextRepository::DeleteCase(double position_in_repository, bool deleted/* = true*/)
{
    int64_t file_position = static_cast<int64_t>(position_in_repository);
    size_t bytes_for_case = GetBytesFromPosition(file_position);

    DeleteCase(file_position, bytes_for_case, deleted, nullptr);
}


void IndexableTextRepository::DeleteCase(const CString& key)
{
    auto [file_position, bytes_for_case] = GetPositionBytesFromKey(key);

    DeleteCase(file_position, bytes_for_case, true, &key);
}


size_t IndexableTextRepository::GetNumberCases() const
{
    EnsureSqlStatementIsPrepared(SqlQueryType::CountNotDeletedKeys, m_stmtCountNotDeletedKeys);
    SQLiteResetOnDestruction rod(*m_stmtCountNotDeletedKeys);

    if( m_stmtCountNotDeletedKeys->Step() == SQLITE_ROW )
        return m_stmtCountNotDeletedKeys->GetColumn<size_t>(0);

    throw DataRepositoryException::SQLiteError();
}
