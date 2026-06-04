#include "stdafx.h"
#include "SQLiteRepository.h"
#include "SQLiteBlobQuestionnaireSerializer.h"
#include "SQLiteDictionarySchemaGenerator.h"
#include "SQLiteDictionarySchemaReconciler.h"
#include "SQLiteErrorWithMessage.h"
#include "SQLiteQuestionnaireSerializer.h"
#include "SQLiteRepositoryIterators.h"
#include "SQLiteSyncStatusEvaluator.h"
#include "SyncBinaryDataUploadManager.h"
#include <zSql/SQLiteHelpers.h>
#include <zUtilO/Interapp.h>
#include <zCaseO/CaseItemReference.h>
#include <sstream>


namespace
{
    // Version of data file database schema. To be used to handle loading
    // files loaded in older versions.
    constexpr int SCHEMA_VERSION = 3;

    constexpr const char* create_indices_sql =
        "CREATE UNIQUE INDEX `cases-id` ON cases(id);\n"
        "CREATE INDEX `cases-deleted-key-file-order` on cases(deleted, key, file_order);\n"
        "CREATE INDEX `cases-last-modified-revision-key` on cases(last_modified_revision, key);\n"
        "CREATE UNIQUE INDEX `vector-clock-case-id-device` ON vector_clock(case_id, device);\n"
        "PRAGMA foreign_keys=ON;\n"
        ;

    bool CreateIndices(sqlite3* pDB)
    {
        return ( sqlite3_exec(pDB, create_indices_sql, nullptr, nullptr, nullptr) == SQLITE_OK );
    }
}


SQLiteRepository::SQLiteRepository(const DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type, DeviceId device_id)
    :   ISyncableDataRepository(type, std::move(case_access), access_type),
        m_db(nullptr),
        m_deviceId(std::move(device_id)),
        m_transactionClientRevision(-1),
        m_transactionStartCount(0),
        m_stmtInsertCase(nullptr),
        m_stmtUpdateCase(nullptr),
        m_stmtSelectCases(nullptr),
        m_stmtCountCases(nullptr),
        m_stmtGetCaseByKey(nullptr),
        m_stmtGetCaseByFileOrder(nullptr),
        m_stmtGetCaseById(nullptr),
        m_stmtContainsCase(nullptr),
        m_stmtModifyDeleteStatus(nullptr),
        m_stmtInsertRevision(nullptr),
        m_stmtInsertLocalRevision(nullptr),
        m_stmtQueryUuidByPosition(nullptr),
        m_stmtUpdateClock(nullptr),
        m_stmtIncrementClock(nullptr),
        m_stmtNewClock(nullptr),
        m_stmtGetClock(nullptr),
        m_stmtGetNotes(nullptr),
        m_stmtClearNotes(nullptr),
        m_stmtUpdateNote(nullptr),
        m_stmtSyncCase(nullptr),
        m_stmtInsertBinarySyncHistory(nullptr),
        m_stmtDeleteBinarySyncHistory(nullptr),
        m_stmtArchiveBinarySyncHistory(nullptr),
        m_stmtRevisionByNumber(nullptr),
        m_stmtIsPrevSync(nullptr),
        m_stmtRevisionByDevice(nullptr),
        m_stmtRevisionsByDeviceSince(nullptr),
        m_stmtCaseIdentifiersFromKey(nullptr),
        m_stmtCaseIdentifiersFromUuid(nullptr),
        m_stmtCaseIdentifiersFromFileOrder(nullptr),
        m_stmtCaseExists(nullptr),
        m_stmtGetPrevFileOrder(nullptr),
        m_stmtSetSyncRevLastId(nullptr),
        m_stmtClearSyncRevLastId(nullptr),
        m_stmtGetFileOrderFromUuid(nullptr)
{
    ModifyCaseAccess(m_caseAccess);
}


SQLiteRepository::SQLiteRepository(std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type, DeviceId device_id)
    :   SQLiteRepository(DataRepositoryType::SQLite, std::move(case_access), access_type, std::move(device_id))
{
}


SQLiteRepository::~SQLiteRepository()
{
    try
    {
        Close();
    }

    catch( const DataRepositoryException::Error& )
    {
    }
}


void SQLiteRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    m_caseAccess = std::move(case_access);

    if( m_questionnaireSerializer != nullptr )
        m_questionnaireSerializer->SetCaseAccess(m_caseAccess, IsReadOnly());

    if( m_db != nullptr )
    {
        ClearPreparedStatements();
        CreatePreparedStatements();
    }
}


void SQLiteRepository::Open(const DataRepositoryOpenFlag open_flag)
{
    const std::string& file_path = m_connectionString.GetFilePath();

    if (!SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), GetFileExtension()))
    {
        throw DataRepositoryException::IOError("The filename %s does not have the correct file extension. Must be '%s'.",
                                               PortableFunctions::PathGetFilename(file_path).c_str(), GetFileExtension());
    }

    bool bCanCreateFile = ( open_flag == DataRepositoryOpenFlag::CreateNew || open_flag == DataRepositoryOpenFlag::OpenOrCreate );

    if( bCanCreateFile && !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(file_path)) )
    {
        throw DataRepositoryException::IOError("The directory does not exist and could not be created: %s",
                                               PortableFunctions::PathGetDirectory(file_path).c_str());
    }

    bool bFileExists = PortableFunctions::FileIsRegular(file_path);

    if (!bFileExists && open_flag == DataRepositoryOpenFlag::OpenMustExist) {
        throw DataRepositoryException::IOError("The data file does not exist: %s", file_path.c_str());
    }

    // create a data file if one doesn't exist (or if clearing/opening in batch output mode, to overwrite what is already on the disk)
    if (!bFileExists || m_accessType == DataRepositoryAccess::BatchOutput || open_flag == DataRepositoryOpenFlag::CreateNew) {
        if (!CreateDatabaseFile()) {
            throw DataRepositoryException::IOError("Could not create a new data file.");
        }
    } else {
        // Open the database
        OpenDatabaseFile();
    }

    if( GetSchemaVersion(m_db) <= 2 )
    {
        // Use blobs for backwards compatability
        m_questionnaireSerializer = std::make_unique<SQLiteBlobQuestionnaireSerializer>(m_db);
    }

    else
    {
        // Relational format
        m_questionnaireSerializer = std::make_unique<SQLiteQuestionnaireSerializer>(m_repositoryId, m_db);
    }

    m_questionnaireSerializer->SetCaseAccess(m_caseAccess, IsReadOnly());

    CreatePreparedStatements();

    // For regular non-batch usage (main entry dict, external dict)
    // we will wrap every WriteCase (or group of WriteCases) in a transaction.
    // In batch mode all operations are wrapped in a few big transactions
    // to get decent performance for writes.
    if (!IsReadOnly() && m_accessType != DataRepositoryAccess::EntryInput && m_accessType != DataRepositoryAccess::ReadWrite) {
        SQLiteRepository::StartTransaction();
    }

    m_transactionClientRevision = -1;
}


void SQLiteRepository::ToggleReadWriteMode()
{
    ASSERT(m_accessType == DataRepositoryAccess::ReadOnly || m_accessType == DataRepositoryAccess::ReadWrite);

    // to resolve sqlite_busy issues when closing the file, add a handler that waits for max of 5 seconds before erroring on close
    sqlite3_busy_timeout(m_db, 5000);
    Close();

    m_accessType = ( m_accessType == DataRepositoryAccess::ReadOnly ) ? DataRepositoryAccess::ReadWrite :
                                                                        DataRepositoryAccess::ReadOnly;
    Open(DataRepositoryOpenFlag::OpenMustExist);
}


void SQLiteRepository::Close()
{
    if( m_db == nullptr )
        return;

    ClearPreparedStatements();

    m_questionnaireSerializer.reset();

    // For batch output create the indices at the end
    // This is faster for bulk writes
    if (m_accessType == DataRepositoryAccess::BatchOutput) {
        if (!CreateIndices(m_db))
            throw SQLiteErrorWithMessage(m_db);
    }

    SQLiteRepository::EndTransaction();

    if( sqlite3_close(m_db) != SQLITE_OK )
        throw SQLiteErrorWithMessage(m_db);

    m_db = nullptr;
}


const char* SQLiteRepository::GetFileExtension() const
{
    return FileExtensions::Data::CSProDB;
}


int SQLiteRepository::OpenSQLiteDatabaseFile(const ConnectionString& connection_string, sqlite3** ppDb, int flags)
{
    return sqlite3_open_v2(connection_string.GetFilePath().c_str(), ppDb, flags, nullptr);
}


int SQLiteRepository::OpenSQLiteDatabase(const ConnectionString& connection_string, sqlite3** ppDb, int flags)
{
    return OpenSQLiteDatabaseFile(connection_string, ppDb, flags);
}


bool SQLiteRepository::CreateDatabaseFile()
{
    const std::string& file_path = m_connectionString.GetFilePath();

    if (PortableFunctions::FileExists(file_path) && !PortableFunctions::FileDelete(file_path))
        return false;

    sqlite3* pDB = nullptr;

    if (OpenSQLiteDatabase(m_connectionString, &pDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) != SQLITE_OK)
        return false;

    // Default page size of 4096 is small. This improves performance with
    // bulk writes.
    sqlite3_exec(pDB, "PRAGMA page_size = 32768", nullptr, nullptr, nullptr);

    if (m_accessType == DataRepositoryAccess::BatchOutput) {
        // These settings improve performance when doing lots of writes
        // to the database however if there is crash the database can become
        // corrupted. In batch output that is not a big deal since
        // we are creating the file from scratch each time.
        sqlite3_exec(pDB, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr);
        sqlite3_exec(pDB, "PRAGMA journal_mode = OFF", nullptr, nullptr, nullptr);

        // Since we create indices at the end in batch and foreign keys rely on indexes
        // we turn off foreign keys until the end after indexes are created
        sqlite3_exec(pDB, "PRAGMA foreign_keys = OFF;", nullptr, nullptr, nullptr);
    }
    else {
        sqlite3_exec(pDB, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    }

    const char* create_static_tables_sql =
        "BEGIN;"
        "CREATE TABLE meta ("
            "schema_version INTEGER NOT NULL,"
            "cspro_version TEXT NOT NULL,"
            "dictionary TEXT NOT NULL,"
            "dictionary_structure TEXT NOT NULL,"
            "dictionary_timestamp INT NOT NULL"
        ");\n"
        "CREATE TABLE file_revisions ("
            "id INTEGER NOT NULL PRIMARY KEY,"
            "device_id TEXT NOT NULL,"
            "timestamp INTEGER NOT NULL default (strftime('%s','now'))"
        ");\n"
        "CREATE TABLE sync_history ("
            "id INTEGER NOT NULL PRIMARY KEY,"
            "file_revision INTEGER NOT NULL,"
            "device_id TEXT NOT NULL,"
            "device_name TEXT,"
            "user_name TEXT,"
            "timestamp INTEGER NOT NULL default (strftime('%s','now')),"
            "universe TEXT NULL,"
            "direction INTEGER NULL,"
            "server_revision TEXT NULL,"
            "partial INTEGER default 0,"
            "last_id TEXT NULL default NULL);\n"
        "CREATE TABLE cases ("
            "id TEXT NOT NULL,"
            "`key` TEXT NOT NULL,"
            "label TEXT,"
            "questionnaire TEXT NOT NULL,"
            "last_modified_revision INTEGER NOT NULL,"
            "deleted INTEGER NOT NULL DEFAULT 0,"
            "file_order REAL NOT NULL UNIQUE,"
            "verified INTEGER NOT NULL DEFAULT 0,"
            "partial_save_mode TEXT NULL,"
            "partial_save_field_name TEXT NULL,"
            "partial_save_level_key TEXT NULL,"
            "partial_save_record_occurrence INTEGER NULL,"
            "partial_save_item_occurrence INTEGER NULL,"
            "partial_save_subitem_occurrence INTEGER NULL,"
            "FOREIGN KEY(last_modified_revision) REFERENCES file_revisions(id)"
        ");\n"
        "CREATE TABLE vector_clock ("
            "case_id TEXT,"
            "device TEXT,"
            "revision INTEGER,"
            "FOREIGN KEY(case_id) REFERENCES cases(id)"
        ");\n"
        "CREATE TABLE notes ("
            "case_id TEXT NOT NULL,"
            "field_name TEXT NOT NULL,"
            "level_key TEXT NOT NULL,"
            "record_occurrence INTEGER NOT NULL,"
            "item_occurrence INTEGER NOT NULL,"
            "subitem_occurrence INTEGER NOT NULL,"
            "content TEXT NOT NULL,"
            "operator_id TEXT NOT NULL,"
            "modified_time INTEGER NOT NULL,"
            "FOREIGN KEY(case_id) REFERENCES cases(id)"
        ");\n"
        "CREATE INDEX `notes-case-id` ON notes(case_id);";

    if (sqlite3_exec(pDB, create_static_tables_sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(file_path);
        return false;
    }

    // Don't create the indices for batch as it slows down database writes
    // We will create them at the end.
    if (m_accessType != DataRepositoryAccess::BatchOutput) {
        if (!CreateIndices(pDB)) {
            sqlite3_close(pDB);
            PortableFunctions::FileDelete(file_path);
            return false;
        }
    }

    SQLiteDictionarySchemaGenerator schema_generator;
    std::ostringstream ss;
    ss << schema_generator.GenerateDictionary(m_caseAccess->GetDataDict());
    std::string create_data_tables_sql = ss.str();

// #define DUMP_SCHEMA
#ifdef DUMP_SCHEMA
    std::ofstream schema_dump;
    const std::string dump_file_name = PortableFunctions::PathRemoveFileExtension(file_path) + "_schema.sql";
    schema_dump.open(dump_file_name);
    schema_dump << ( create_static_tables_sql + strlen("BEGIN;") )
                << create_data_tables_sql;
    schema_dump.close();
#endif

    if (sqlite3_exec(pDB, create_data_tables_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        const std::string error = sqlite3_errmsg(pDB);
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(file_path);
        throw DataRepositoryException::SQLiteError(error.c_str());
    }

    const CDataDict& dictionary = m_caseAccess->GetDataDict();

    const char* insertVersions = "INSERT INTO meta(schema_version, cspro_version, dictionary, "
                                                  "dictionary_structure, dictionary_timestamp) "
                                                  "values(?,?,?,?,?);";
    if (SQLiteStatement(pDB, insertVersions)
        .Bind(1, SCHEMA_VERSION)
        .Bind(2, Versioning::NumberDetailedText)
        .Bind(3, dictionary.GetJson())
        .Bind(4, dictionary.GetStructureMd5())
        .Bind(5, dictionary.GetFileModifiedTime())
        .Step() != SQLITE_DONE) {
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(file_path);
        return false;
    }

    if (sqlite3_exec(pDB, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(file_path);
        return false;
    }

    m_db = pDB;
    return true;
}


void SQLiteRepository::CreatePreparedStatements()
{
    sqlite3_prepare_v2(m_db, "SELECT device, revision FROM vector_clock WHERE case_id=?", -1, &m_stmtGetClock, nullptr);

    // create the ReadCase statements
    auto prepare_read_case_statement = [&](auto& statement, const auto& where_clause)
    {
        std::ostringstream read_sql;
        read_sql << "SELECT id, file_order, deleted";

        if( m_caseAccess->GetUsesCaseLabels() )
            read_sql << ",label";

        if( m_caseAccess->GetUsesStatuses() )
        {
            read_sql << ",verified, partial_save_mode, partial_save_field_name, partial_save_level_key, "
                   "partial_save_record_occurrence, partial_save_item_occurrence, partial_save_subitem_occurrence";
        }

        read_sql << " FROM cases WHERE " << where_clause << " LIMIT 1";
        sqlite3_prepare_v2(m_db, read_sql.str().c_str(), -1, &statement, nullptr);
    };

    prepare_read_case_statement(m_stmtGetCaseByKey, "deleted = 0 AND key=? ORDER BY file_order");
    prepare_read_case_statement(m_stmtGetCaseByFileOrder, "file_order=?");
    prepare_read_case_statement(m_stmtGetCaseById, "id=?");

    if( m_caseAccess->GetUsesNotes() )
    {
        sqlite3_prepare_v2(m_db, "SELECT field_name, level_key, record_occurrence, item_occurrence, subitem_occurrence, "
                                 "content, operator_id, modified_time FROM notes WHERE case_id=?", -1, &m_stmtGetNotes, nullptr);
    }

    // Update for use in entry and sync where case already exists
    std::ostringstream sql;
    sql <<
        "UPDATE cases SET key=@key, label=@dky, last_modified_revision=@rev, deleted=@del,"
        "file_order=COALESCE(@ord, (SELECT file_order FROM cases WHERE id = @id ), (SELECT MAX(file_order) + 1 FROM cases), 1),"
        "verified=@ver, partial_save_mode=@psm, partial_save_field_name=@psf, partial_save_level_key=@psl,"
        "partial_save_record_occurrence=@psr, partial_save_item_occurrence=@psi, partial_save_subitem_occurrence=@pss"
        " WHERE id=@id";

    if (sqlite3_prepare_v2(m_db, sql.str().c_str(), -1, &m_stmtUpdateCase, nullptr) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }

    // Insert for use when case does not already exist
    sql.clear();
    sql.str("");
    sql <<
        "INSERT INTO cases(id, key, label, questionnaire, last_modified_revision, deleted, file_order, "
        "verified, partial_save_mode, partial_save_field_name, partial_save_level_key, partial_save_record_occurrence, partial_save_item_occurrence, partial_save_subitem_occurrence)"
        " VALUES(@id , @key , @dky, '', @rev , @del, COALESCE(@ord, (SELECT MAX(file_order) + 1 FROM cases), 1) , @ver , @psm , @psf , @psl , @psr , @psi , @pss)";

    if (sqlite3_prepare_v2(m_db, sql.str().c_str(), -1, &m_stmtInsertCase, nullptr) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }

    if (sqlite3_prepare_v2(m_db, "SELECT file_order FROM cases WHERE id=?", -1, &m_stmtGetFileOrderFromUuid, nullptr) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }

    if (sqlite3_prepare_v2(m_db, "SELECT id, file_order FROM cases WHERE deleted = 0 AND key=? LIMIT 1", -1, &m_stmtCaseIdentifiersFromKey, nullptr) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }
}


void SQLiteRepository::ClearPreparedStatements()
{
    safe_sqlite3_finalize(m_stmtInsertCase);
    safe_sqlite3_finalize(m_stmtUpdateCase);
    safe_sqlite3_finalize(m_stmtSelectCases);
    safe_sqlite3_finalize(m_stmtCountCases);
    safe_sqlite3_finalize(m_stmtGetCaseByKey);
    safe_sqlite3_finalize(m_stmtGetCaseByFileOrder);
    safe_sqlite3_finalize(m_stmtGetCaseById);
    safe_sqlite3_finalize(m_stmtContainsCase);
    safe_sqlite3_finalize(m_stmtModifyDeleteStatus);
    safe_sqlite3_finalize(m_stmtInsertRevision);
    safe_sqlite3_finalize(m_stmtInsertLocalRevision);
    safe_sqlite3_finalize(m_stmtQueryUuidByPosition);
    safe_sqlite3_finalize(m_stmtUpdateClock);
    safe_sqlite3_finalize(m_stmtIncrementClock);
    safe_sqlite3_finalize(m_stmtNewClock);
    safe_sqlite3_finalize(m_stmtGetClock);
    safe_sqlite3_finalize(m_stmtGetNotes);
    safe_sqlite3_finalize(m_stmtClearNotes);
    safe_sqlite3_finalize(m_stmtUpdateNote);
    safe_sqlite3_finalize(m_stmtSyncCase);
    safe_sqlite3_finalize(m_stmtInsertBinarySyncHistory);
    safe_sqlite3_finalize(m_stmtDeleteBinarySyncHistory);
    safe_sqlite3_finalize(m_stmtArchiveBinarySyncHistory);
    safe_sqlite3_finalize(m_stmtRevisionByNumber);
    safe_sqlite3_finalize(m_stmtIsPrevSync);
    safe_sqlite3_finalize(m_stmtRevisionByDevice);
    safe_sqlite3_finalize(m_stmtRevisionsByDeviceSince);
    safe_sqlite3_finalize(m_stmtCaseIdentifiersFromKey);
    safe_sqlite3_finalize(m_stmtCaseIdentifiersFromUuid);
    safe_sqlite3_finalize(m_stmtCaseIdentifiersFromFileOrder);
    safe_sqlite3_finalize(m_stmtCaseExists);
    safe_sqlite3_finalize(m_stmtGetPrevFileOrder);
    safe_sqlite3_finalize(m_stmtSetSyncRevLastId);
    safe_sqlite3_finalize(m_stmtClearSyncRevLastId);
    safe_sqlite3_finalize(m_stmtGetFileOrderFromUuid);

    if( m_syncStatusEvaluator != nullptr )
        m_syncStatusEvaluator->ClearPreparedStatements();
}


double SQLiteRepository::CreateInsertPosition(const double insert_before_position_in_repository)
{
    SQLiteStatement getPrevFileOrder(m_db, m_stmtGetPrevFileOrder,
        "SELECT `file_order` FROM `cases` WHERE `file_order` < ? ORDER BY `file_order` DESC LIMIT 1;"
    );

    getPrevFileOrder.Bind(1, insert_before_position_in_repository);

    double prevPos;
    if (getPrevFileOrder.Step() == SQLITE_ROW) {
        prevPos = getPrevFileOrder.GetColumn<double>(0);
    } else {
        prevPos = 0; // before must be the first case in repo
    }

    return ( insert_before_position_in_repository + prevPos ) / 2;
}


std::unique_ptr<CDataDict> SQLiteRepository::ReadDictionaryFromDatabase(sqlite3* const db)
{
    SQLiteStatement getDictStatement(db, "SELECT dictionary FROM meta");

    if( getDictStatement.Step() != SQLITE_ROW )
    {
        sqlite3_close(db);
        throw DataRepositoryException::IOError("Invalid file format. Missing dictionary.");
    }

    const std::string dict_contents = getDictStatement.GetColumn<std::string>(0);
    std::string_view dict_contents_sv = dict_contents;

    const TextEncoding text_encoding(dict_contents_sv);

    if( text_encoding.UsesBom() )
        dict_contents_sv.remove_prefix(text_encoding.GetBomLength());

    if( SO::IsWhitespace(dict_contents_sv) )
        throw DataRepositoryException::IOError("Invalid file format. Dictionary is blank.");

    try
    {
        auto dictionary = std::make_unique<CDataDict>();
        dictionary->OpenFromText(dict_contents_sv);
        return dictionary;
    }

    catch( const CSProException& exception )
    {
        throw DataRepositoryException::IOError("Data file could not be read by this version of CSPro. "
                                               "It may have been created with a newer version of CSPro or it may be corrupt. "
                                               "Try opening it in the latest version of CSPro. %s",
                                               exception.what());
    }
}


std::unique_ptr<CDataDict> SQLiteRepository::GetEmbeddedDictionary(const ConnectionString& connection_string)
{
    std::unique_ptr<CDataDict> dictionary;
    sqlite3* db = nullptr;

    if( OpenSQLiteDatabaseFile(connection_string, &db, SQLITE_OPEN_READONLY) == SQLITE_OK )
    {
        dictionary = ReadDictionaryFromDatabase(db);
        sqlite3_close(db);
    }

    return dictionary;
}


void SQLiteRepository::OpenDatabaseFile()
{
    sqlite3* pDB = nullptr;

    int flags = IsReadOnly() ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE;

    int result = OpenSQLiteDatabase(m_connectionString, &pDB, flags);
    if (result != SQLITE_OK) {
        throw DataRepositoryException::IOError("Invalid file format. Not a valid database.");
    }

    int iSchemaVersion;
    try {
        iSchemaVersion = GetSchemaVersion(pDB);
    }
    catch( const DataRepositoryException::Error& exception ) {
        sqlite3_close(pDB);
        throw exception;
    }

    if (iSchemaVersion > SCHEMA_VERSION) {
        sqlite3_close(pDB);
        throw DataRepositoryException::IOError("Data file was created by a newer version of CSPro and is not compatible with this version.");
    }

    // Older versions of schema did not have device_name column. Add it in if it is not there.
    bool needToAddDeviceUserNameToSyncHistory = iSchemaVersion == 2 && MissingDeviceNameColumnInSyncHistory(pDB);

    bool needToAddBinaryTable = iSchemaVersion == 3 && MissingBinaryTable(pDB);
    bool needToAddBinarySyncHistoryTable = iSchemaVersion == 3 && MissingBinarySyncHistoryTable(pDB);

    if (iSchemaVersion == 1 || needToAddDeviceUserNameToSyncHistory || needToAddBinaryTable || needToAddBinarySyncHistoryTable) {

        // Can't migrate a read-only database, so need to reopen read-write to migrate
        MakeDatabaseTemporarilyWriteable(&pDB);

        if (iSchemaVersion == 1)
            MigrateFromSchemaVersion1(pDB);
        if (needToAddDeviceUserNameToSyncHistory)
            AddDeviceNameUserNameColumnsToSyncHistory(pDB);
        if (needToAddBinaryTable)
            AddBinaryTable(pDB);
        if (needToAddBinarySyncHistoryTable)
            AddBinarySyncHistoryTables(pDB);
        EndMakeDatabaseTemporarilyWriteable(&pDB);
    }

    // Verify that dictionary matches original dictionary used to create the file
    if (iSchemaVersion > 2) {
        try {
            ReconcileDictionaries(&pDB);
        } catch (DataRepositoryException::Error&) {
            sqlite3_close(pDB);
            throw;
        }
    }

    // Update the dictionary stored in DB so that we have latest
    if (flags != SQLITE_OPEN_READONLY) {
        try {
            UpdateDictionary(pDB);
        }
        catch( const DataRepositoryException::Error& ) {
            sqlite3_close(pDB);
            throw;
        }
    }

    m_db = pDB;

    if (m_accessType != DataRepositoryAccess::BatchOutput)
        sqlite3_exec(pDB, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
}


void SQLiteRepository::DeleteRepository()
{
    Close();

    if( !PortableFunctions::FileDelete(m_connectionString.GetFilePath()) )
        throw DataRepositoryException::DeleteRepositoryError();
}


bool SQLiteRepository::ContainsCase(const std::string& key)
{
    const char* containsCaseSql = "SELECT 1 FROM cases WHERE deleted = 0 AND key=? LIMIT 1";
    return SQLiteStatement(m_db, m_stmtContainsCase, containsCaseSql).
        Bind(1, key).
        Step() == SQLITE_ROW;
}


void SQLiteRepository::PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository)
{
    auto bind_and_step = [](auto& statement, const auto& bind_value)
    {
        statement.Bind(1, bind_value);

        if (statement.Step() != SQLITE_ROW)
            throw DataRepositoryException::CaseNotFound();
    };

    if (!key.empty()) {
        SQLiteStatement getCaseIdentifiersFromKey(m_stmtCaseIdentifiersFromKey);
        bind_and_step(getCaseIdentifiersFromKey, key);
        uuid = getCaseIdentifiersFromKey.GetColumn<std::string>(0);
        position_in_repository = getCaseIdentifiersFromKey.GetColumn<double>(1);
    }

    else if (!uuid.empty()) {
        SQLiteStatement getCaseIdentifiersFromUuid(m_db, m_stmtCaseIdentifiersFromUuid, "SELECT key, file_order FROM cases WHERE id=? LIMIT 1");
        bind_and_step(getCaseIdentifiersFromUuid, uuid);
        key = getCaseIdentifiersFromUuid.GetColumn<std::string>(0);
        position_in_repository = getCaseIdentifiersFromUuid.GetColumn<double>(1);
    }

    else {
        SQLiteStatement getCaseIdentifiersFromFileOrder(m_db, m_stmtCaseIdentifiersFromFileOrder, "SELECT key, id FROM cases WHERE file_order=? LIMIT 1");
        bind_and_step(getCaseIdentifiersFromFileOrder, position_in_repository);
        key = getCaseIdentifiersFromFileOrder.GetColumn<std::string>(0);
        uuid = getCaseIdentifiersFromFileOrder.GetColumn<std::string>(1);
    }
}


DataRepositoryUniqueCaseIdentifer SQLiteRepository::GetUniqueCaseIdentifer(const CaseKey& case_key)
{
    return case_key.GetPositionInRepository();
}


std::optional<CaseKey> SQLiteRepository::FindCaseKey(const CaseIterationMethod iteration_method, const CaseIterationOrder iteration_order,
                                                     const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    const CaseIteratorSettings iterator_settings(CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order, start_parameters);

    std::unique_ptr<SQLiteStatement> statement = GetKeySearchIteratorStatement(iterator_settings, 0, 1,
        "SELECT `cases`.`key`, `cases`.`file_order` "
        "FROM `cases` "
        "JOIN ( %s ) AS `filtered_cases` ON `cases`.`file_order` = `filtered_cases`.`file_order`"
    );

    std::optional<CaseKey> case_key;

    if( statement->Step() == SQLITE_ROW )
        case_key.emplace(statement->GetColumn<std::string>(0), statement->GetColumn<double>(1));

    return case_key;
}


void SQLiteRepository::ReadCase(Case& data_case, const std::string& key)
{
    SQLiteStatement getCaseByKey(m_stmtGetCaseByKey);
    getCaseByKey.Bind(1, key);

    if( getCaseByKey.Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    ReadCaseFromDatabase(data_case, getCaseByKey);
}


void SQLiteRepository::ReadCase(Case& data_case, const double position_in_repository)
{
    SQLiteStatement getCaseByFileOrder(m_stmtGetCaseByFileOrder);
    getCaseByFileOrder.Bind(1, position_in_repository);

    if( getCaseByFileOrder.Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    ReadCaseFromDatabase(data_case, getCaseByFileOrder);
}


void SQLiteRepository::ReadCaseByUuid(Case& data_case, const std::string& uuid)
{
    SQLiteStatement getCaseById(m_stmtGetCaseById);
    getCaseById.Bind(1, uuid);

    if( getCaseById.Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    ReadCaseFromDatabase(data_case, getCaseById);
}


bool SQLiteRepository::ReadCaseFromUuid(std::unique_ptr<Case>& data_case, const std::string& uuid)
{
    SQLiteStatement getCaseById(m_stmtGetCaseById);
    getCaseById.Bind(1, uuid);

    if( getCaseById.Step() != SQLITE_ROW )
        return false;

    if( data_case == nullptr )
        data_case = m_caseAccess->CreateCase();

    ReadCaseFromDatabase(*data_case, getCaseById);

    return true;
}


void SQLiteRepository::WriteCase(Case& data_case, const WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    bool new_case;
    std::optional<double> position_in_repository;
    std::string uuid;

    if( m_accessType == DataRepositoryAccess::BatchOutput ||
        m_accessType == DataRepositoryAccess::BatchOutputAppend )
    {
        ASSERT(write_case_parameter == nullptr);

        // Batch is always treated as a new case
        // since we write into new output file
        // TODO: what about duplicates in batch append????
        new_case = true;

        // Preserve the UUID if there is one so that batch apps will keep
        // the same UUID in input and output files.
        uuid = data_case.GetUuid();
    }

    // this will be from CSEntry or from Data.writeCase
    else if( write_case_parameter != nullptr )
    {
        if( write_case_parameter->IsModifyParameter() )
        {
            new_case = false;

            position_in_repository = write_case_parameter->GetPositionInRepository();

            // reuse the UUID for this case
            uuid = GetUuidByPosition(*position_in_repository);
        }

        else
        {
            ASSERT(write_case_parameter->IsInsertParameter());
            new_case = true;

            const double insert_before_position_in_repository = write_case_parameter->GetPositionInRepository();
            position_in_repository = CreateInsertPosition(insert_before_position_in_repository);
        }
    }

    else if( m_accessType == DataRepositoryAccess::EntryInput )
    {
        // CSEntry modifications and insertions are handled above ( write_case_parameter != nullptr )
        new_case = true;
    }

    else if( m_accessType == DataRepositoryAccess::ReadWrite )
    {
        // From writecase we update the case based on the case id (key)
        // and not the UUID. This way logic like:
        //      ID = 1; writecase(DICT); ID = 2; writecase(DICT)
        // will generate two distinct cases even though it ends
        // up being two consecutive calls to WriteCase() with
        // the same UUID.
        SQLiteStatement getUuidPosFromKey(m_stmtCaseIdentifiersFromKey);
        getUuidPosFromKey.Bind(1, data_case.GetKey());

        switch( getUuidPosFromKey.Step() )
        {
            // There is already a case with this case id, use the same UUID to overwrite it
            case SQLITE_ROW:
                new_case = false;
                uuid = getUuidPosFromKey.GetColumn<std::string>(0);
                position_in_repository = getUuidPosFromKey.GetColumn<double>(1);
                break;

            // No existing case with this case id so generate a new case
            case SQLITE_DONE:
                new_case = true;
                break;

            default:
                throw SQLiteErrorWithMessage(m_db);
        }
    }

    else
    {
        ASSERT(m_accessType == DataRepositoryAccess::BatchInput ||
               m_accessType == DataRepositoryAccess::ReadOnly);

        throw DataRepositoryException::WriteAccessRequired();
    }

    ASSERT(new_case || position_in_repository != 0);
    data_case.SetPositionInRepository(position_in_repository.value_or(0));

    ASSERT(new_case || !uuid.empty());
    data_case.SetUuid(!uuid.empty() ? std::move(uuid) : CreateUuid());

    int64_t revision;

    if( m_transactionStartCount > 0 )
    {
        // Only start a new revision if we haven't added
        // one since we opened the file.
        if( m_transactionClientRevision == -1 )
            m_transactionClientRevision = AddFileRevision();

        revision = m_transactionClientRevision;
    }

    else
    {
        // Start a new revision for each write (in batch we use a single revision)
        revision = AddFileRevision();
    }

    SQLiteRepository::StartTransaction();

    if( new_case )
    {
        int insertResult = InsertCase(data_case, revision);

        if( insertResult == SQLITE_CONSTRAINT && m_accessType == DataRepositoryAccess::BatchOutputAppend )
        {
            // Duplicate UUID in batch append mode, create a new UUID to allow duplicate
            data_case.SetUuid(CreateUuid());
            insertResult = InsertCase(data_case, revision);

            if( data_case.GetCaseConstructionReporter() != nullptr )
                data_case.GetCaseConstructionReporter()->DuplicateUuid(data_case);
        }

        if( insertResult != SQLITE_DONE )
            throw SQLiteErrorWithMessage(m_db);

        // Update the file pos in the case to make caching work
        if( m_accessType == DataRepositoryAccess::ReadWrite )
            UpdateFilePosition(data_case);

        data_case.GetVectorClock().increment(m_deviceId);
        InsertVectorClock(data_case);
        WriteNotes(data_case);
    }

    else
    {
        if( UpdateCase(data_case, revision) != SQLITE_DONE )
            throw SQLiteErrorWithMessage(m_db);

        IncrementVectorClock(data_case.GetUuid());

        if( write_case_parameter == nullptr ||
            !write_case_parameter->IsModifyParameter() ||
            write_case_parameter->AreNotesModified() )
        {
            ClearNotes(data_case);
            WriteNotes(data_case);
        }
    }

    SQLiteRepository::EndTransaction();
    CommitTransactionIfTooBig();
}


void SQLiteRepository::CommitTransactionIfTooBig()
{
    constexpr int MaxNumberSqlInsertsInOneTransaction = 2500;
    if (++m_iInsertInTransactionCounter == MaxNumberSqlInsertsInOneTransaction) {
        sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
        sqlite3_exec(m_db, "BEGIN", nullptr, nullptr, nullptr);
        m_iInsertInTransactionCounter = 0;
    }
}


void SQLiteRepository::ClearNotes(const Case& data_case)
{
    SQLiteStatement clearOldNotesStatement(m_db, m_stmtClearNotes, "DELETE FROM notes where case_id=?");
    clearOldNotesStatement.Bind(1, data_case.GetUuid());
    if (clearOldNotesStatement.Step() != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);
}


void SQLiteRepository::WriteNotes(const Case& data_case)
{
    // Insert notes
    SQLiteStatement updateNotesStatement(m_db, m_stmtUpdateNote,
        "INSERT INTO notes(case_id,field_name,level_key,record_occurrence,item_occurrence,subitem_occurrence,content,operator_id,modified_time)"
        "VALUES( @case, @field, @levelkey, @rec, @item, @subitem, @cont, @opid, @time)");
    for( const Note& note : data_case.GetNotes() ) {
        const NamedReference& named_reference = note.GetNamedReference();
        // the current schema has the occurrences as NOT NULL, but they could be; for now, occurrence-less
        // references will be serialized as 0, but if the schema changes, they should be NULL
        const std::vector<size_t>& one_based_occurrences = named_reference.GetOneBasedOccurrences();
        ASSERT(one_based_occurrences.empty() || one_based_occurrences.size() == 3);

        updateNotesStatement
            .Bind("@case", data_case.GetUuid())
            .Bind("@field", named_reference.GetName())
            .Bind("@levelkey", named_reference.GetLevelKey())
            .Bind("@rec", one_based_occurrences.empty() ? 0 : one_based_occurrences[0])
            .Bind("@item", one_based_occurrences.empty() ? 0 : one_based_occurrences[1])
            .Bind("@subitem", one_based_occurrences.empty() ? 0 : one_based_occurrences[2])
            .Bind("@cont", note.GetContent())
            .Bind("@opid", note.GetOperatorId())
            .Bind("@time", (int64_t)note.GetModifiedDateTime());

        if (updateNotesStatement.Step() != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }
        updateNotesStatement.Reset();
    }
}


std::string SQLiteRepository::GetUuidByPosition(const double position_in_repository)
{
    SQLiteStatement stmt(m_db, m_stmtQueryUuidByPosition,
        "SELECT `id` FROM `cases` WHERE `file_order` = ?;"
    );

    stmt.Bind(1, position_in_repository);

    if( stmt.Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    return stmt.GetColumn<std::string>(0);
}


void SQLiteRepository::IncrementVectorClock(const std::string& uuid)
{
    SQLiteStatement updateClockStatement(m_db, m_stmtIncrementClock,
        "INSERT OR REPLACE INTO vector_clock(case_id, device, revision)"
        "VALUES( @id , @dev , "
        "COALESCE((SELECT revision + 1 FROM vector_clock WHERE case_id = @id AND device = @dev ), 1))");
    updateClockStatement
        .Bind("@id", uuid)
        .Bind("@dev", m_deviceId);

    if (updateClockStatement.Step() != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);
}


void SQLiteRepository::DeleteCase(double position_in_repository, bool deleted/* = true*/)
{
    SQLiteRepository::StartTransaction();

    int64_t revision;

    if (m_transactionStartCount) {

        // Only start a new revision if we haven't added
        // one since we opened the file.
        if (m_transactionClientRevision == -1) {
            m_transactionClientRevision = AddFileRevision();
        }
        revision = m_transactionClientRevision;

    } else {
        // Start a new revision for each write (in batch we use a single revision)
        revision = AddFileRevision();
    }

    SQLiteStatement stmtModifyDeleteStatus(m_db, m_stmtModifyDeleteStatus, "UPDATE cases SET deleted=?, last_modified_revision=? WHERE file_order=?");
    stmtModifyDeleteStatus.Bind(1, deleted ? 1 : 0)
                          .Bind(2, revision)
                          .Bind(3, position_in_repository);

    if (stmtModifyDeleteStatus.Step() != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);

    if (sqlite3_changes(m_db) == 0)
        throw DataRepositoryException::CaseNotFound();

    const std::string uuid = GetUuidByPosition(position_in_repository);
    IncrementVectorClock(uuid);

    SQLiteRepository::EndTransaction();
    CommitTransactionIfTooBig();
}


size_t SQLiteRepository::GetNumberCases()
{
    SQLiteStatement statement(m_db, m_stmtCountCases, "SELECT COUNT(*) FROM cases WHERE deleted = 0");

    if( statement.Step() == SQLITE_ROW )
        return statement.GetColumn<size_t>(0);

    throw DataRepositoryException::SQLiteError();
}


size_t SQLiteRepository::GetNumberCases(const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    if( case_status == CaseIterationCaseStatus::NotDeletedOnly && start_parameters == nullptr )
        return GetNumberCases();

    const CaseIteratorSettings iterator_settings(case_status, std::nullopt, std::nullopt, start_parameters);

    std::unique_ptr<SQLiteStatement> statement = GetKeySearchIteratorStatement(iterator_settings, 0, SIZE_MAX,
        "SELECT COUNT(*) "
        "FROM `cases` "
        "JOIN ( %s ) AS `filtered_cases` ON `cases`.`file_order` = `filtered_cases`.`file_order`"
    );

    if( statement->Step() == SQLITE_ROW )
        return statement->GetColumn<size_t>(0);

    throw DataRepositoryException::SQLiteError();
}


void SQLiteRepository::WriteIteratorSelectFromSql(std::stringstream& sql, CaseIterationContent iteration_content) const
{
    const bool get_case_note_for_case_summary = ( iteration_content == CaseIterationContent::CaseSummary &&
                                                  m_caseAccess->GetUsesNotes() && CaseIterator::RequiresCaseNote() );

    sql << "SELECT "
        << ( ( iteration_content == CaseIterationContent::Case ) ? "`cases`.`id`" : "`cases`.`key`" )
        << ", `cases`.`file_order`";

    if( iteration_content != CaseIterationContent::CaseKey )
    {
        sql << ", `cases`.`deleted`";

        if( m_caseAccess->GetUsesCaseLabels() )
            sql << ", `cases`.`label`";

        if( m_caseAccess->GetUsesStatuses() )
        {
            sql << ", `cases`.`verified`, `cases`.`partial_save_mode`";

            if( iteration_content == CaseIterationContent::Case )
            {
                sql << ", `cases`.`partial_save_field_name`, `cases`.`partial_save_level_key`, `cases`.`partial_save_record_occurrence`,"
                            "`cases`.`partial_save_item_occurrence`, `cases`.`partial_save_subitem_occurrence`";
            }
        }

        if( iteration_content == CaseIterationContent::Case )
            sql << ", `cases`.`questionnaire`";

        else if( get_case_note_for_case_summary )
            sql << ", `notes`.`content`";
    }

    sql << " FROM `cases` ";
}


std::unique_ptr<CaseIterator> SQLiteRepository::CreateIterator(const CaseIterationContent iteration_content,
                                                               const CaseIteratorSettings& iterator_settings,
                                                               const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    const bool get_case_note_for_case_summary = ( iteration_content == CaseIterationContent::CaseSummary &&
                                                  m_caseAccess->GetUsesNotes() &&
                                                  CaseIterator::RequiresCaseNote() );

    std::stringstream sql;
    WriteIteratorSelectFromSql(sql, iteration_content);
    sql << "JOIN ( %s ) AS `filtered_cases` ON `cases`.`file_order` = `filtered_cases`.`file_order` ";

    if( get_case_note_for_case_summary )
    {
        sql << "LEFT OUTER JOIN `notes` ON `cases`.`id` = `notes`.`case_id` AND "
               "`notes`.`field_name` = '" << m_caseAccess->GetDataDict().GetName().c_str() << "' AND `notes`.`operator_id`='' ";
    }

    return std::make_unique<SQLiteRepositoryCaseIterator>(
        *this,
        iteration_content,
        GetKeySearchIteratorStatement(iterator_settings, offset, limit, sql.str().c_str()),
        &iterator_settings
    );
}


std::unique_ptr<SQLiteStatement> SQLiteRepository::GetKeySearchIteratorStatement(const CaseIteratorSettings& iterator_settings,
                                                                                 const size_t offset, const size_t limit,
                                                                                 const char* const base_sql) const
{
    std::string order_by_text;

    if( iterator_settings.GetMethod().has_value() )
    {
        const CaseIterationMethod iteration_method = *iterator_settings.GetMethod();
        const std::optional<CaseIterationOrder>& iteration_order = iterator_settings.GetOrder();

        order_by_text = FormatText("ORDER BY %s %s ",
            ( iteration_method == CaseIterationMethod::KeyOrder ) ? "`cases`.`key`" : "`cases`.`file_order`",
            ( ( iteration_order == CaseIterationOrder::Ascending )  ? "ASC" :
              ( iteration_order == CaseIterationOrder::Descending ) ? "DESC" :
                                                                      "" ));
    }

    const std::string limit_text = FormatText("LIMIT %d OFFSET %d ",
                                              ( limit == SIZE_MAX ) ? -1 : static_cast<int>(limit),
                                              static_cast<int>(offset));

    std::string where_text;

    auto add_to_where_text = [&](const auto& condition)
    {
        where_text.append(where_text.empty() ? "WHERE (" : "AND (")
                  .append(condition)
                  .append(") ");
    };

    // process any filters
    const CaseIteratorParameters* const start_parameters = iterator_settings.GetParameters();
    bool use_key_prefix = false;
    bool use_operators = false;

    if( start_parameters != nullptr )
    {
        // use the key prefix if it is set and is not empty
        if( start_parameters->key_prefix.has_value() && !start_parameters->key_prefix->empty() )
        {
            use_key_prefix = true;
            where_text = "WHERE `cases`.`key` >= ? AND `cases`.`key` < ? ";

            use_operators = std::holds_alternative<std::string>(start_parameters->first_key_or_position) ?
                !std::get<std::string>(start_parameters->first_key_or_position).empty() :
                ( std::get<double>(start_parameters->first_key_or_position) != -1 );
        }

        else
        {
            use_operators = true;
        }

        if( use_operators )
        {
            add_to_where_text(FormatText("%s %s ?",
                                         std::holds_alternative<std::string>(start_parameters->first_key_or_position) ? "`cases`.`key`" : "`cases`.`file_order`",
                                         ToString(start_parameters->start_type)));
        }
    }

    // filter on case properties
    if( iterator_settings.GetStatus() != CaseIterationCaseStatus::All )
    {
        add_to_where_text("`cases`.`deleted` = 0");

        if( iterator_settings.GetStatus() == CaseIterationCaseStatus::PartialsOnly )
        {
            add_to_where_text("`cases`.`partial_save_mode` IS NOT NULL");
        }

        else if( iterator_settings.GetStatus() == CaseIterationCaseStatus::DuplicatesOnly )
        {
            add_to_where_text("`cases`.`key` IN ( SELECT `cases`.`key` FROM `cases` WHERE `cases`.`deleted` = 0 GROUP BY `cases`.`key` HAVING COUNT(*) > 1 )");
        }
    }

    // generate the complete SQL statement
    const std::string filter_sql = FormatText("SELECT `cases`.`file_order` FROM `cases` %s %s %s ",
                                              where_text.c_str(), order_by_text.c_str(), limit_text.c_str());

    std::string sql = FormatText(base_sql, filter_sql.c_str());
    sql.append(order_by_text);

    auto statement = std::make_unique<SQLiteStatement>(m_db, sql);

    if( use_key_prefix )
    {
        statement->Bind(1, *start_parameters->key_prefix);
        statement->Bind(2, SQLiteHelpers::GetTextPrefixBoundary(*start_parameters->key_prefix));
    }

    if( use_operators )
    {
        const int operator_argument_index = use_key_prefix ? 3 : 1;

        if( std::holds_alternative<std::string>(start_parameters->first_key_or_position) )
        {
            statement->Bind(operator_argument_index, std::get<std::string>(start_parameters->first_key_or_position));
        }

        else
        {
            statement->Bind(operator_argument_index, std::get<double>(start_parameters->first_key_or_position));
        }
    }

    return statement;
}


void SQLiteRepository::UpdateDictionary(sqlite3* pDB)
{
    // only update the dictionary if it has been modified
    SQLiteStatement get_timestamp_statement(pDB, "SELECT dictionary_timestamp FROM meta");

    // this column only exists starting starting with CSPro 7.5
    bool meta_table_has_timestamp = ( get_timestamp_statement.Step() == SQLITE_ROW );
    const CDataDict& dictionary = m_caseAccess->GetDataDict();

    if (meta_table_has_timestamp && get_timestamp_statement.GetColumn<int64_t>(0) == dictionary.GetFileModifiedTime()) {
        return;
    }

    const char* updateDictionary = meta_table_has_timestamp ? "UPDATE meta SET dictionary=?, dictionary_structure=?, dictionary_timestamp=?" :
                                                              "UPDATE meta SET dictionary=?";

    SQLiteStatement update_dictionary_statement(pDB, updateDictionary);
    update_dictionary_statement.Bind(1, dictionary.GetJson());

    if (meta_table_has_timestamp) {
        update_dictionary_statement.Bind(2, dictionary.GetStructureMd5());
        update_dictionary_statement.Bind(3, dictionary.GetFileModifiedTime());
    }

    if (update_dictionary_statement.Step() != SQLITE_DONE) {
        throw SQLiteErrorWithMessage(pDB);
    }
}


void SQLiteRepository::ReconcileDictionaries(sqlite3** pDB)
{
    // only reconcile the dictionaries if the structure has been modified
    const std::string embedded_dict_sig = GetDictionaryStructureMd5(*pDB);
    if (!embedded_dict_sig.empty() && embedded_dict_sig == m_caseAccess->GetDataDict().GetStructureMd5())
        return;

    std::unique_ptr<CDataDict> pOriginalDict = ReadDictionaryFromDatabase(*pDB);
    SQLiteDictionarySchemaReconciler reconciler(*pDB, *pOriginalDict, m_caseAccess.get());
    if (reconciler.NeedsReconcile()) {
        MakeDatabaseTemporarilyWriteable(pDB);
        reconciler.Reconcile(*pDB);
        EndMakeDatabaseTemporarilyWriteable(pDB);
    }
}


std::string SQLiteRepository::GetDictionaryStructureMd5(sqlite3* const db)
{
    SQLiteStatement get_structure_statement(db, "SELECT dictionary_structure FROM meta");

    // this column only exists starting starting with CSPro 7.5
    if (get_structure_statement.Step() != SQLITE_ROW) {
        return std::string();
    }
    else {
        return get_structure_statement.GetColumn<std::string>(0);
    }
}


int SQLiteRepository::GetSchemaVersion(sqlite3* const pDB) const
{
    SQLiteStatement getMetaStatement(pDB, "SELECT schema_version FROM meta");
    if (getMetaStatement.Step() != SQLITE_ROW) {
        // The meta table could be missing if this isn't really a csdb file but also could be
        // if the file is currently open as output to batch edit program which has deleted
        // and recreated file but has not yet flushed transaction that creates the meta table.
        throw DataRepositoryException::IOError("Unable to read CSPro DB file. Could be an invalid file, a corrupt file or it could be in use by another program.");
    }

    return getMetaStatement.GetColumn<int>(0);
}


void SQLiteRepository::MigrateFromSchemaVersion1(sqlite3* pDB)
{
    const char *sql = "BEGIN TRANSACTION;"
        "ALTER TABLE file_revisions RENAME TO temp_file_revisions;"
        "CREATE TABLE file_revisions("
        "   id INTEGER NOT NULL PRIMARY KEY,"
        "   device_id TEXT NOT NULL,"
        "   timestamp INTEGER NOT NULL default (strftime('%s', 'now')));"
        "INSERT INTO file_revisions"
        "   SELECT"
        "   id, device_id, timestamp"
        "   FROM"
        "   temp_file_revisions;"
        "CREATE TABLE sync_history("
        "   id INTEGER NOT NULL PRIMARY KEY,"
        "   file_revision INTEGER NOT NULL,"
        "   device_id TEXT NOT NULL,"
        "   timestamp INTEGER NOT NULL default (strftime('%s','now')),"
        "   universe TEXT NULL,"
        "   direction INTEGER NULL,"
        "   server_revision TEXT NULL,"
        "   partial INTEGER default 0,"
        "   last_id TEXT NULL default NULL,"
        "   FOREIGN KEY(file_revision) REFERENCES file_revisions(id));"
        "DROP TABLE temp_file_revisions;"
        "UPDATE meta SET schema_version=2;"
        "COMMIT;";

    int rc = sqlite3_exec(pDB, sql, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }
}


void SQLiteRepository::AddDeviceNameUserNameColumnsToSyncHistory(sqlite3* pDB)
{
    const char* sql = "BEGIN TRANSACTION;"
        "ALTER TABLE sync_history RENAME TO temp_sync_history;"
        "CREATE TABLE sync_history ("
        "id INTEGER NOT NULL PRIMARY KEY,"
        "file_revision INTEGER NOT NULL,"
        "device_id TEXT NOT NULL,"
        "device_name TEXT,"
        "user_name TEXT,"
        "timestamp INTEGER NOT NULL default (strftime('%s','now')),"
        "universe TEXT NULL,"
        "direction INTEGER NULL,"
        "server_revision TEXT NULL,"
        "partial INTEGER default 0,"
        "last_id TEXT NULL default NULL,"
        "FOREIGN KEY(file_revision) REFERENCES file_revisions(id));"
        "INSERT INTO sync_history"
        "   SELECT"
        "   id, file_revision, device_id, device_id, null, timestamp, universe, direction, server_revision, partial, last_id"
        "   FROM"
        "   temp_sync_history;"
        "DROP TABLE temp_sync_history;"
        "COMMIT;";

    int rc = sqlite3_exec(pDB, sql, nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }
}


bool SQLiteRepository::MissingDeviceNameColumnInSyncHistory(sqlite3* pDB)
{
    SQLiteStatement check_device_name(pDB, "SELECT 1 FROM pragma_table_info('sync_history') WHERE name = 'device_name'");
    return check_device_name.Step() != SQLITE_ROW;
}


bool SQLiteRepository::MissingBinaryTable(sqlite3* pDB)
{
    SQLiteStatement check_binary_table(pDB, "SELECT 1 FROM sqlite_master WHERE type='table' AND name='binary-data';");
    return check_binary_table.Step() != SQLITE_ROW;
}


void SQLiteRepository::AddBinaryTable(sqlite3* pDB)
{
    SQLiteDictionarySchemaGenerator schema_generator;
    SQLiteSchema::Schema binary_table_schema = schema_generator.GenerateBinaryTable();
    binary_table_schema.CreateDatabase(pDB);
}


bool SQLiteRepository::MissingBinarySyncHistoryTable(sqlite3* pDB)
{
    //if the binary-sync-history table is missing then the archive table is also missing
    SQLiteStatement check_binary_sync_history_table(pDB, "SELECT 1 FROM sqlite_master WHERE type='table' AND name='binary-sync-history';");
    return check_binary_sync_history_table.Step() != SQLITE_ROW;
}


void SQLiteRepository::AddBinarySyncHistoryTables(sqlite3* pDB)
{
    //create the binary sync history table and the archive table to move items when revision on server is not found and full sync is done
    SQLiteDictionarySchemaGenerator schema_generator;
    SQLiteSchema::Schema binary_table_sync_history_schema = schema_generator.GenerateBinarySyncHistoryTable();
    binary_table_sync_history_schema.CreateDatabase(pDB);
    SQLiteSchema::Schema binary_table_sync_history_archive_schema = schema_generator.GenerateBinarySyncHistoryArchiveTable();
    binary_table_sync_history_archive_schema.CreateDatabase(pDB);
}


void SQLiteRepository::MakeDatabaseTemporarilyWriteable(sqlite3** db)
{
    if (IsReadOnly()) {
        sqlite3_close(*db);
        if (OpenSQLiteDatabase(m_connectionString, db, SQLITE_OPEN_READWRITE) != SQLITE_OK) {
            throw DataRepositoryException::IOError("Failed to reopen database as writeable.");
        }
    }
}


void SQLiteRepository::EndMakeDatabaseTemporarilyWriteable(sqlite3** db)
{
    if (IsReadOnly()) {
        sqlite3_close(*db);
        if (OpenSQLiteDatabase(m_connectionString, db, SQLITE_OPEN_READWRITE) != SQLITE_OK) {
            throw DataRepositoryException::IOError("Failed to reopen database after making writeable.");
        }
    }
}


void SQLiteRepository::StartSync(DeviceId server_device_id, std::string remote_device_name, std::string username, const SyncDirection direction,
                                 std::string universe, const bool use_remote_case_on_conflict)
{
    if( m_accessType == DataRepositoryAccess::BatchInput )
        throw DataRepositoryException::WriteAccessRequired();

    if( m_accessType == DataRepositoryAccess::BatchOutput || m_accessType == DataRepositoryAccess::BatchOutputAppend )
        throw DataRepositoryException::NotValidInBatchOutput();

    if( m_accessType == DataRepositoryAccess::ReadOnly )
    {
        // fix to allow readonly external file to be synced by reopening in read write mode
        try
        {
            // to resolve sqlite_busy issues when closing the file add a handler that waits for max of 5 seconds before erroring on close
            sqlite3_busy_timeout(m_db, 5000);
            Close();

            // reopen the file in readwrite mode to allow sync writes
            m_accessType = DataRepositoryAccess::ReadWrite;
            m_caseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(m_caseAccess->GetDataDict());
            Open(DataRepositoryOpenFlag::OpenMustExist);
        }

        catch(...)
        {
            throw DataRepositoryException::IOError("Unable to close and reopen file read-write to sync previously read-only file.");
        }
    }

    m_currentSyncStats = DataSyncStatistics();

    // Save these here - they get written to DB when cases are received/sent that way
    // we don't record anything if we never contact the server.
    m_currentSyncParams.current_sync_id = 0; // Gets set later when we write to DB
    m_currentSyncParams.current_client_revision = 0; // Gets set later when we write to DB
    m_currentSyncParams.remote_device_id = std::move(server_device_id);
    m_currentSyncParams.remote_device_name = std::move(remote_device_name);
    m_currentSyncParams.username = std::move(username);
    m_currentSyncParams.direction = direction;
    m_currentSyncParams.universe = std::move(universe);
    m_currentSyncParams.use_remote_case_on_conflict = use_remote_case_on_conflict;
    m_currentSyncParams.server_revision.clear();
}


int SQLiteRepository::SyncCasesFromRemote(const std::vector<std::shared_ptr<Case>>& cases_received, const std::string& server_revision)
{
    m_currentSyncParams.server_revision = server_revision;

    if( m_currentSyncParams.current_sync_id == 0 )
    {
        // First cases received this sync, haven't added revision to database yet
        if( m_transactionStartCount != 0 )
        {
            if( m_transactionClientRevision == -1 )
                m_transactionClientRevision = AddFileRevision();

            m_currentSyncParams.current_client_revision = m_transactionClientRevision;
        }

        else
        {
            m_currentSyncParams.current_client_revision = AddFileRevision();
        }

        m_currentSyncParams.current_sync_id = AddSyncHistoryEntry(SyncHistoryEntry::SyncState::PartialGet, SO::Empty_string);
    }

    std::unique_ptr<Case> local_case;

    for( const std::shared_ptr<Case>& remote_case : cases_received )
    {
        ++m_currentSyncStats.cases_received;

        if( ReadCaseFromUuid(local_case, remote_case->GetUuid()) )
        {
            // if the case exists both locally and remotely, compare using vector clocks...
            ASSERT(local_case != nullptr);

            // Local case is more recent, do not update
            if( remote_case->GetVectorClock() < local_case->GetVectorClock() )
            {
                ++m_currentSyncStats.cases_newer_in_repository;
            }

            // Update is newer, replace the local case
            else if( local_case->GetVectorClock() < remote_case->GetVectorClock() )
            {
                constexpr bool NewCase = false;
                SyncCase(*remote_case, m_currentSyncParams.current_client_revision, NewCase);
                ++m_currentSyncStats.cases_newer_on_remote;
            }

            // Conflict - neither clock is greater
            else if( local_case->GetVectorClock() != remote_case->GetVectorClock() )
            {
                ++m_currentSyncStats.cases_with_conflicts;

                // Make a new case with merged clocks
                VectorClock mergedClock = local_case->GetVectorClock();
                mergedClock.merge(remote_case->GetVectorClock());

                // The "use_remote_case_on_conflict" flag will be true on the server and
                // false on the client so that the client always wins conflicts.
                Case* const baseCaseForResolve = m_currentSyncParams.use_remote_case_on_conflict ? remote_case.get() : local_case.get();
                VectorClock baseCaseClock = baseCaseForResolve->GetVectorClock();
                baseCaseForResolve->SetVectorClock(std::move(mergedClock));

                constexpr bool NewCase = false;
                SyncCase(*baseCaseForResolve, m_currentSyncParams.current_client_revision, NewCase);
                baseCaseForResolve->SetVectorClock(std::move(baseCaseClock));
            }
        }

        else
        {
            // No local version of the case so it must be new, add it to repo
            ++m_currentSyncStats.cases_not_in_repository;

            constexpr bool NewCase = true;
            SyncCase(*remote_case, m_currentSyncParams.current_client_revision, NewCase);
        }
    }

    if( m_caseAccess->GetCaseMetadata().UsesBinaryData() )
        AddBinaryItemsSyncHistory(cases_received, m_currentSyncParams.current_sync_id);

    m_currentSyncParams.server_revision = server_revision;

    SetSyncRevisionPartial(m_currentSyncParams.current_sync_id, SyncHistoryEntry::SyncState::PartialGet,
                           m_currentSyncParams.server_revision, cases_received.empty() ? SO::Empty_string : cases_received.back()->GetUuid(),
                           m_currentSyncParams.current_client_revision);

    ASSERT(m_currentSyncParams.current_client_revision == static_cast<int>(m_currentSyncParams.current_client_revision));
    return static_cast<int>(m_currentSyncParams.current_client_revision);
}


void SQLiteRepository::MarkCasesSentToRemote(const cs::span<const Case* const> cases_sent, const SyncBinaryDataUploadManager* const sync_binary_data_upload_manager,
                                             const std::string& server_revision, const int client_revision)
{
    m_currentSyncParams.current_client_revision = client_revision;
    m_currentSyncParams.server_revision = server_revision;

    if( m_currentSyncParams.current_sync_id == 0 )
    {
        // First cases received this sync, haven't added revision to database yet
        m_currentSyncParams.current_sync_id = AddSyncHistoryEntry(SyncHistoryEntry::SyncState::PartialPut, SO::Empty_string);
    }

    const std::string& last_case_uuid = cases_sent.empty() ? SO::Empty_string :
                                                             cases_sent.back()->GetUuid();

    SetSyncRevisionPartial(m_currentSyncParams.current_sync_id, SyncHistoryEntry::SyncState::PartialPut,
                           m_currentSyncParams.server_revision, last_case_uuid, m_currentSyncParams.current_client_revision);

    m_currentSyncStats.cases_sent += cases_sent.size();

    if( sync_binary_data_upload_manager != nullptr )
    {
        sync_binary_data_upload_manager->ForeachBinaryCaseItemInChunk(
            [&](const std::string& signature)
            {
                AddBinaryItemsSyncHistory(signature, m_currentSyncParams.current_sync_id);
            });
    }
}


void SQLiteRepository::AddBinaryItemsSyncHistory(const std::string& signature, const int sync_id)
{
    ASSERT(BinaryDataAccessor::IsValidSignature(signature));

    // Insert into binary-sync-history
    SQLiteStatement insertBinarySyncHistoryStatement(m_db, m_stmtInsertBinarySyncHistory,
        "INSERT INTO `binary-sync-history`(`binary-data-signature`,`sync-history-id`)"
        "VALUES(?,?)");

    insertBinarySyncHistoryStatement.Bind(1, signature)
                                    .Bind(2, sync_id);

    if( insertBinarySyncHistoryStatement.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);
}


void SQLiteRepository::AddBinaryItemsSyncHistory(const std::vector<std::shared_ptr<Case>>& cases_received, const int sync_id)
{
    ASSERT(m_caseAccess->GetCaseMetadata().UsesBinaryData());

    for( const std::shared_ptr<Case>& data_case : cases_received )
    {
        data_case->ForeachDefinedBinaryCaseItem(
            [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
            {
                const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

                // only add entries received during this sync that were loaded, which will only happen once per entry, when loaded
                // in SQLiteBinaryItemSerializer::InsertContent, even if that entry appears multiple times in a case or cases
                if( binary_data_accessor.IsDefinedAndContentLoaded() )
                {
                    AddBinaryItemsSyncHistory(binary_data_accessor.GetSignature(), sync_id);
                }

                else
                {
#ifdef _DEBUG
                    SQLiteStatement stmt(m_db, "SELECT `sync-history-id` FROM `binary-sync-history` WHERE `binary-data-signature` = ? LIMIT 1;");
                    stmt.Bind(1, binary_data_accessor.GetSignature());
                    ASSERT(stmt.Step() == SQLITE_ROW && stmt.GetColumn<int>(0) <= sync_id);
#endif
                }
            });
    }
}


void SQLiteRepository::ClearBinarySyncHistory(const DeviceId& server_device_id, const int client_revision/* = -1*/)
{
    //This function is called when sync client cannot find a revision on the server and has to do a full sync
    //This should be only for binary items that have been synced to this / from server

    //Archive the binary sync history when its a full resync by moving any syncs including the binary items associated to gets as well
    // as the server may have deleted all the binary items and we need to resend them
    //all our binaries in this case again as we cannot find the last revision on the server
    SQLiteStatement archiveInvalidBinarySyncHistoryEntries(m_db, m_stmtArchiveBinarySyncHistory,
        "INSERT INTO `binary-sync-history-archive` (`binary-sync-history-id`, `binary-data-signature`, `sync-history-id`)"
        " SELECT `binary-sync-history`.`id`, `binary-data-signature`,"
        " `sync-history-id` FROM `binary-sync-history` JOIN `sync_history` ON `binary-sync-history`.`sync-history-id` = `sync_history`.`id`"
        " WHERE `sync_history`.`device_id` = ? AND `sync_history`.`file_revision` >= ?");

    archiveInvalidBinarySyncHistoryEntries.Bind(1, server_device_id)
                                          .Bind(2, client_revision);

    if( archiveInvalidBinarySyncHistoryEntries.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);

    //Clear the binary sync history when its a full resync by deleting any syncs as we need  to resend
    //all our binaries in this case again as we cannot find the last revision on the server
    SQLiteStatement deleteInvalidBinarySyncHistoryEntries(m_db, m_stmtDeleteBinarySyncHistory,
        "DELETE FROM `binary-sync-history` WHERE `binary-sync-history`.`id` IN(SELECT `binary-sync-history`.`id` FROM `binary-sync-history` JOIN `sync_history` ON `binary-sync-history`.`sync-history-id` = `sync_history`.`id`"
        " WHERE `sync_history`.`device_id` = ? AND `sync_history`.`file_revision` >= ?)");

    deleteInvalidBinarySyncHistoryEntries.Bind(1, server_device_id)
                                         .Bind(2, client_revision);

    if( deleteInvalidBinarySyncHistoryEntries.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);
}


void SQLiteRepository::EndSync()
{
    SetSyncRevisionComplete(m_currentSyncParams.current_sync_id);
}


DataSyncStatistics SQLiteRepository::GetLastSyncStats() const
{
    return m_currentSyncStats;
}


void SQLiteRepository::SyncCase(Case& data_case, const int64_t client_revision, const bool bNewCase)
{
    // Ensure that position is not copied from remote repo
    data_case.SetPositionInRepository(0);

    if (bNewCase) {
        if (InsertCase(data_case, client_revision) != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }
        InsertVectorClock(data_case);
    } else {
        if (UpdateCase(data_case, client_revision) != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }
        UpdateVectorClock(data_case);
        ClearNotes(data_case);
    }

    WriteNotes(data_case);
}


void SQLiteRepository::InsertVectorClock(const Case& data_case)
{
    for ( const DeviceId& deviceId : data_case.GetVectorClock().getAllDevices() ) {
        SQLiteStatement updateClockStatement(m_db, m_stmtNewClock,
            "INSERT INTO vector_clock(case_id, device, revision)"
            "VALUES(@id , @dev , @rev)");
        updateClockStatement
            .Bind("@id", data_case.GetUuid())
            .Bind("@dev", deviceId)
            .Bind("@rev", data_case.GetVectorClock().getVersion(deviceId));

        if (updateClockStatement.Step() != SQLITE_DONE)
            throw SQLiteErrorWithMessage(m_db);
    }
}


void SQLiteRepository::UpdateVectorClock(const Case& data_case)
{
    for ( const DeviceId& deviceId : data_case.GetVectorClock().getAllDevices() ) {
        SQLiteStatement updateClockStatement(m_db, m_stmtUpdateClock,
            "INSERT OR REPLACE INTO vector_clock(case_id, device, revision)"
            "VALUES(@id , @dev , @rev)");
        updateClockStatement
            .Bind("@id", data_case.GetUuid())
            .Bind("@dev", deviceId)
            .Bind("@rev", data_case.GetVectorClock().getVersion(deviceId));

        if (updateClockStatement.Step() != SQLITE_DONE)
            throw SQLiteErrorWithMessage(m_db);
    }
}


int SQLiteRepository::InsertOrUpdateCase(const Case& data_case, int64_t revision, sqlite3_stmt* pStmt)
{
    SQLiteStatement insertOrUpdateCase(pStmt);

    insertOrUpdateCase
        .Bind("@id", data_case.GetUuid())
        .Bind("@key", data_case.GetKey())
        .Bind("@dky", data_case.GetCaseLabel())
        .Bind("@rev", revision)
        .Bind("@ver", data_case.GetVerified())
        .Bind("@del", data_case.GetDeleted());

    // Only bind file order if it is set to something valid, otherwise
    // SQL insert statement will pick appropriate value
    if (data_case.GetPositionInRepository() > 0)
        insertOrUpdateCase.Bind("@ord", data_case.GetPositionInRepository());
    else
        insertOrUpdateCase.BindNull("@ord");

    BindPartialSave(data_case, insertOrUpdateCase);

    int case_result = insertOrUpdateCase.Step();
    if (case_result != SQLITE_DONE) {
        return case_result;
    }

    m_questionnaireSerializer->WriteQuestionnaire(data_case, revision);

    return case_result;
}


int SQLiteRepository::InsertCase(const Case& data_case, int64_t revision)
{
    return InsertOrUpdateCase(data_case, revision, m_stmtInsertCase);
}


int SQLiteRepository::UpdateCase(const Case& data_case, int64_t revision)
{
    return InsertOrUpdateCase(data_case, revision, m_stmtUpdateCase);
}


void SQLiteRepository::BindPartialSave(const Case& data_case, SQLiteStatement &insertCase)
{
    if (data_case.IsPartial())
        insertCase.Bind("@psm", (int)data_case.GetPartialSaveMode());

    else
        insertCase.BindNull("@psm");

    if (data_case.GetPartialSaveCaseItemReference() != nullptr) {
        const CaseItemReference& partial_save_case_item_reference = *data_case.GetPartialSaveCaseItemReference();
        const std::vector<size_t>& one_based_occurrences = partial_save_case_item_reference.GetOneBasedOccurrences();
        ASSERT(one_based_occurrences.size() == 3);

        insertCase
            .Bind("@psf", partial_save_case_item_reference.GetName())
            .Bind("@psl", partial_save_case_item_reference.GetLevelKey())
            .Bind("@psr", one_based_occurrences[0])
            .Bind("@psi", one_based_occurrences[1])
            .Bind("@pss", one_based_occurrences[2]);
    } else {
        insertCase
            .BindNull("@psf")
            .BindNull("@psl")
            .BindNull("@psr")
            .BindNull("@psi")
            .BindNull("@pss");
    }
}


std::unique_ptr<CaseIterator> SQLiteRepository::GetCasesModifiedSinceRevisionIterator(const int client_revision, const std::string& last_case_uuid, const std::string& universe,
                                                                                      const size_t limit/* = std::numeric_limits<size_t>::max()*/, size_t* const out_case_count/* = nullptr*/, int* const out_last_client_revision/* = nullptr*/,
                                                                                      const cs::cref_optional<DeviceId> ignore_gets_from_device_id/* = std::nullopt*/,
                                                                                      const cs::cref_optional<std::vector<std::string>> revisions_to_exclude/* = std::nullopt*/)
{
    std::stringstream where_sql;
    std::optional<std::string> universe_to_bind;
    bool bind_ignore_gets_from_device_id = false;
    std::optional<std::vector<std::string>> revision_parameters_to_bind;

    where_sql << "(last_modified_revision > @rev OR ( @lid IS NOT NULL AND @lid != '' AND last_modified_revision = @rev AND id > @lid )) ";

    if( !universe.empty() )
    {
        where_sql << "AND key LIKE @uni ";
        universe_to_bind = universe + "%";
    }

    if( ignore_gets_from_device_id.has_value() && !ignore_gets_from_device_id->empty() )
    {
        bind_ignore_gets_from_device_id = true;
        where_sql << "AND last_modified_revision NOT IN (SELECT file_revision FROM sync_history WHERE device_id=@dev AND direction = 2) ";
    }

    if( revisions_to_exclude.has_value() && !revisions_to_exclude->empty() )
    {
        revision_parameters_to_bind.emplace();

        where_sql << "AND last_modified_revision NOT IN (";

        for( size_t i = 0; i < revisions_to_exclude->size(); ++i )
        {
            if( i != 0 )
                where_sql << ',';

            where_sql << revision_parameters_to_bind->emplace_back(FormatText("@ir%d", static_cast<int>(i)));
        }

        where_sql << ") ";
    }

    const std::string evaluated_where_sql = where_sql.str();

    auto bind_shared_options = [&](SQLiteStatement& statement, const bool add_limit)
    {
        statement.Bind("@rev", client_revision)
                 .Bind("@lid", last_case_uuid);

        if( add_limit )
            statement.Bind("@lim", limit);

        if( universe_to_bind.has_value() )
            statement.Bind("@uni", *universe_to_bind);

        if( bind_ignore_gets_from_device_id )
            statement.Bind("@dev", *ignore_gets_from_device_id);

        if( revision_parameters_to_bind.has_value() )
        {
            ASSERT(revision_parameters_to_bind->size() == revisions_to_exclude->size());

            for( size_t i = 0; i < revisions_to_exclude->size(); ++i )
                statement.Bind(revision_parameters_to_bind->at(i).c_str(), revisions_to_exclude->at(i));
        }
    };

    if( out_case_count != nullptr )
    {
        SQLiteStatement countStmt(m_db, SO::Concatenate("SELECT COUNT(*) FROM cases WHERE ", evaluated_where_sql));
        bind_shared_options(countStmt, false);

        countStmt.Step();
        *out_case_count = countStmt.GetColumn<size_t>(0);
    }

    if( out_last_client_revision != nullptr )
    {
        SQLiteStatement maxStmt(m_db, SO::Concatenate("SELECT COALESCE(MAX(last_modified_revision), (SELECT MAX(last_modified_revision) FROM cases)) "
                                                      "FROM (SELECT last_modified_revision FROM cases "
                                                      "WHERE ", evaluated_where_sql,
                                                      "ORDER BY last_modified_revision "
                                                      "LIMIT @lim)"));
        bind_shared_options(maxStmt, true);

        maxStmt.Step();
        *out_last_client_revision = maxStmt.GetColumn<int>(0);
    }

    std::stringstream sql;
    WriteIteratorSelectFromSql(sql, CaseIterationContent::Case);

    auto statement = std::make_unique<SQLiteStatement>(m_db, SO::Concatenate(sql.str(),
                                                                             "WHERE ", evaluated_where_sql,
                                                                             "ORDER BY last_modified_revision, id "
                                                                             "LIMIT @lim"));
    bind_shared_options(*statement, true);

    return std::make_unique<SQLiteRepositoryCaseIterator>(*this, CaseIterationContent::Case, std::move(statement), nullptr);
}


void SQLiteRepository::AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, const DeviceId& server_device_id, std::vector<std::string>& signatures_to_sync)
{
    ASSERT(m_caseAccess->GetCaseMetadata().UsesBinaryData());

    // To determine which binary items need to be uploaded in the next sync upload we just need to find all the binary items that do NOT have a corresponding
    // entry in the binary-sync-history entry that matches or exceeds the last_modified_revision of the item. This means left joining cases, case-binary-data,
    // binary-data, binary-sync-history and file-revisions filtered by the universe and the server (to ignore syncs with other servers), and then grouping the
    // results by binary-data.id, taking the max(sync_history.file_revision) and filtering the results to keep only
    // those rows where max(sync_history.file_revision) is null or less than binary-data.file_revision.

    // Get all the binary signatures for this case that have not been synced to the given device since the last modified revision
    // if the binary itemes have never been synced to the given device return all the md5s for the case

    const char* join_where_sql = "";

    if( !server_device_id.empty() )
    {
        // select binary items that have either never been sent or sent to this server and check if the max file revision sent is less than
        // the current version of the binary item and ensure that it is sent;
        // to avoid sending binary items we got from this server - ignore binary items from this server where the file version have not changed since
        join_where_sql = " WHERE last_modified_revision NOT IN (SELECT file_revision FROM sync_history WHERE device_id=@dev AND direction = 2) ";
    }

    const std::string sql = SO::Concatenate("SELECT signature"
                                            " FROM (SELECT `case-binary-data`.`binary-data-signature` AS `signature`,`last_modified_revision`,`last_modified_revision`,`file_revision`"
                                            " FROM `case-binary-data` JOIN `binary-data` ON `binary-data`.signature = `case-binary-data`.`binary-data-signature` AND `case-id` = @id"
                                            " LEFT JOIN( `binary-sync-history` JOIN `sync_history` ON `sync_history`.`id` = `binary-sync-history`.`sync-history-id` AND `device_id` = @dev)"
                                            " ON `case-binary-data`.`binary-data-signature` = `binary-sync-history`.`binary-data-signature`",
                                            join_where_sql,
                                            ") AS T1"
                                            " WHERE ( `file_revision` IS NULL OR `file_revision` < last_modified_revision ) GROUP BY `T1`.`signature`");

    SQLiteStatement case_binary_signatures_statement(m_db, sql);
    case_binary_signatures_statement.Bind("@id", data_case.GetUuid())
                                    .Bind("@dev", server_device_id);

    int result;

    while( ( result = case_binary_signatures_statement.Step() ) == SQLITE_ROW )
        signatures_to_sync.emplace_back(case_binary_signatures_statement.GetColumn<std::string>(0));

    if( result != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);
}


SyncHistoryEntry SQLiteRepository::CreateSyncHistoryEntry(SQLiteStatement& stmt)
{
    // For legacy files with no device name use device id
    DeviceId this_device_id = stmt.GetColumn<DeviceId>(2);
    std::string this_device_name = stmt.IsColumnNull(3) ? this_device_id : stmt.GetColumn<std::string>(3);

    return SyncHistoryEntry(
        stmt.GetColumn<int>(0),
        stmt.GetColumn<int>(1),
        std::move(this_device_id),
        std::move(this_device_name),
        static_cast<SyncDirection>(stmt.GetColumn<int>(4)),
        stmt.GetColumn<std::string>(5),
        stmt.GetColumn<int64_t>(6),
        stmt.GetColumn<std::string>(7),
        static_cast<SyncHistoryEntry::SyncState>(stmt.GetColumn<int>(8)),
        stmt.GetColumn<std::string>(9)
    );
}


std::optional<SyncHistoryEntry> SQLiteRepository::GetLastSyncForDevice(const DeviceId& device_id, const SyncDirection direction) const
{
    ASSERT(!device_id.empty());
    ASSERT(direction != SyncDirection::Both);

    SQLiteStatement statement(m_db, m_stmtRevisionByDevice,
        "SELECT id, file_revision, device_id, device_name, direction, universe, timestamp, server_revision, partial, last_id "
        "FROM sync_history "
        "WHERE device_id=? AND direction=? "
        "ORDER BY id DESC "
        "LIMIT 1"
    );

    statement.Bind(1, device_id)
             .Bind(2, static_cast<int>(direction));

    const int result = statement.Step();

    if( result == SQLITE_DONE )
    {
        return std::nullopt;
    }

    else if( result == SQLITE_ROW )
    {
        return CreateSyncHistoryEntry(statement);
    }

    else
    {
        throw SQLiteErrorWithMessage(m_db);
    }
}


std::vector<SyncHistoryEntry> SQLiteRepository::GetSyncHistory(const DeviceId& device_id/* = DeviceId()*/, const std::optional<SyncDirection> direction/* = std::nullopt*/,
                                                               const std::optional<int> start_serial_number/* = std::nullopt*/, const size_t limit/* = std::numeric_limits<size_t>::max()*/)
{
    ASSERT(direction != SyncDirection::Both);
    ASSERT(start_serial_number != 0);

    SQLiteStatement statement(m_db, m_stmtRevisionsByDeviceSince,
        "SELECT id, file_revision, device_id, device_name, direction, universe, timestamp, server_revision, partial, last_id "
        "FROM sync_history "
        "WHERE id >= @id AND (@dev='' OR device_id=@dev) AND (@dir = 3 OR @dir = direction) "
        "ORDER BY id DESC "
        "LIMIT @li"
    );

    statement.Bind("@id", start_serial_number.value_or(0))
             .Bind("@dev", device_id)
             .Bind("@dir", static_cast<int>(direction.value_or(SyncDirection::Both)))
             .Bind("@li", limit);

    std::vector<SyncHistoryEntry> entries;
    int result;

    while( ( result = statement.Step() ) == SQLITE_ROW )
        entries.emplace_back(CreateSyncHistoryEntry(statement));

    if( result != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);

    return entries;
}


bool SQLiteRepository::IsValidClientRevision(const int client_revision) const
{
    SQLiteStatement statement(m_db, m_stmtRevisionByNumber, "SELECT 1 FROM file_revisions WHERE id=?");
    statement.Bind(1, client_revision);

    const int result = statement.Step();

    return ( result == SQLITE_DONE ) ? false :
           ( result == SQLITE_ROW )  ? true :
                                       throw SQLiteErrorWithMessage(m_db);
}


bool SQLiteRepository::IsPreviousSync(const int client_revision, const DeviceId& device_id) const
{
    SQLiteStatement statement(m_db, m_stmtIsPrevSync, "SELECT 1 FROM sync_history WHERE file_revision=? AND device_id=?");
    statement.Bind(1, client_revision)
             .Bind(2, device_id);

    const int result = statement.Step();

    return ( result == SQLITE_DONE ) ? false :
           ( result == SQLITE_ROW )  ? true :
                                       throw SQLiteErrorWithMessage(m_db);
}


int SQLiteRepository::AddSyncHistoryEntry(const SyncHistoryEntry::SyncState state, const std::string& last_case_uuid)
{
    // reset any cached sync information
    m_syncStatusEvaluator.reset();

    SQLiteStatement statement(m_db, m_stmtInsertRevision, "INSERT INTO sync_history (file_revision, device_id,device_name,user_name,universe,direction,server_revision,partial,last_id) "
                                                          "VALUES (?,?,?,?,?,?,?,?,?)");
    statement.Bind(1, m_currentSyncParams.current_client_revision)
             .Bind(2, m_currentSyncParams.remote_device_id)
             .Bind(3, m_currentSyncParams.remote_device_name)
             .Bind(4, m_currentSyncParams.username)
             .Bind(5, m_currentSyncParams.universe)
             .Bind(6, static_cast<int>(m_currentSyncParams.direction))
             .Bind(7, m_currentSyncParams.server_revision)
             .Bind(8, static_cast<int>(state))
             .Bind(9, last_case_uuid);

    if( statement.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);

    return static_cast<int>(sqlite3_last_insert_rowid(m_db));
}


int64_t SQLiteRepository::AddFileRevision()
{
    SQLiteStatement statement(m_db, m_stmtInsertLocalRevision, "INSERT INTO file_revisions (device_id) VALUES (?)");
    statement.Bind(1, m_deviceId);

    if( statement.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);

    return sqlite3_last_insert_rowid(m_db);
}


void SQLiteRepository::SetSyncRevisionPartial(const int sync_id, const SyncHistoryEntry::SyncState state, const std::string& server_revision,
                                              const std::string& last_case_uuid, const int64_t client_revision)
{
    SQLiteStatement statement(m_db, m_stmtSetSyncRevLastId, "UPDATE sync_history SET partial=?, last_id=?, server_revision=?, file_revision=? WHERE id=?");
    statement.Bind(1, static_cast<int>(state))
             .Bind(2, last_case_uuid)
             .Bind(3, server_revision)
             .Bind(4, client_revision)
             .Bind(5, sync_id);

    if( statement.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);
}


void SQLiteRepository::SetSyncRevisionComplete(const int sync_id)
{
    SQLiteStatement statement(m_db, m_stmtClearSyncRevLastId, "UPDATE sync_history SET partial=0,last_id=NULL WHERE id=?");
    statement.Bind(1, sync_id);

    if( statement.Step() != SQLITE_DONE )
        throw SQLiteErrorWithMessage(m_db);
}


void SQLiteRepository::StartTransaction()
{
    if( m_transactionStartCount == 0 )
    {
        if( sqlite3_exec(m_db, "BEGIN", nullptr, nullptr, nullptr) != SQLITE_OK )
            throw SQLiteErrorWithMessage(m_db);

        m_iInsertInTransactionCounter = 0;
    }

    ++m_transactionStartCount;
}


void SQLiteRepository::EndTransaction()
{
    --m_transactionStartCount;

    if( m_transactionStartCount == 0 )
    {
        m_transactionClientRevision = -1;

        if( sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK )
            throw SQLiteErrorWithMessage(m_db);
    }
}


void SQLiteRepository::UpdateFilePosition(Case& data_case)
{
    SQLiteStatement get_file_pos(m_stmtGetFileOrderFromUuid);
    get_file_pos.Bind(1, data_case.GetUuid());

    if( get_file_pos.Step() != SQLITE_ROW )
        throw SQLiteErrorWithMessage(m_db);

    data_case.SetPositionInRepository(get_file_pos.GetColumn<double>(0));
}



template<typename CF>
auto SQLiteRepository::DoWithSyncStatusEvaluator(const CF& callback_function)
{
    try
    {
        if( m_syncStatusEvaluator == nullptr )
            m_syncStatusEvaluator = std::make_unique<SyncStatusEvaluator>(*this);

        return callback_function();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch( const std::exception& exception )
    {
        // rethrow as a DataRepositoryException::Error
        throw DataRepositoryException::Error(exception.what());
    }
}


std::optional<double> SQLiteRepository::GetSyncTime(const SharableString& device_identifier, const SharableString& case_uuid)
{
    return DoWithSyncStatusEvaluator(
        [&] { return m_syncStatusEvaluator->GetSyncTime(device_identifier, case_uuid); }
    );
}
