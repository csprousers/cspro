#include "stdafx.h"
#include "SQLiteRepository.h"
#include "SQLiteBinaryDataReader.h"
#include "SQLiteBlobQuestionnaireSerializer.h"
#include "SQLiteBlobQuestionnaireSerializer.h"
#include "SQLiteDictionarySchemaGenerator.h"
#include "SQLiteDictionarySchemaReconciler.h"
#include "SQLiteErrorWithMessage.h"
#include "SQLiteQuestionnaireSerializer.h"
#include "SQLiteRepositoryIterators.h"
#include "SyncJsonBinaryDataReader.h"
#include <SQLite/SQLiteHelpers.h>
#include <zUtilO/Interapp.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/CaseItemReference.h>
#include <sstream>


namespace {
    // Version of data file database schema. To be used to handle loading
    // files loaded in older versions.
    const int SCHEMA_VERSION = 3;

    const char* create_indices_sql =
        "CREATE UNIQUE INDEX `cases-id` ON cases(id);\n"
        "CREATE INDEX `cases-deleted-key-file-order` on cases(deleted, key, file_order);\n"
        "CREATE INDEX `cases-last-modified-revision-key` on cases(last_modified_revision, key);\n"
        "CREATE UNIQUE INDEX `vector-clock-case-id-device` ON vector_clock(case_id, device);\n"
        "PRAGMA foreign_keys=ON;\n"
        ;

    bool CreateIndices(sqlite3* pDB)
    {
        return sqlite3_exec(pDB, create_indices_sql, NULL, NULL, NULL) == SQLITE_OK;
    }
}


SQLiteRepository::SQLiteRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type, DeviceId deviceId) :
    ISyncableDataRepository(type, std::move(case_access), access_type),
    m_db(nullptr),
    m_deviceId(deviceId),
    m_transaction_file_revision(-1),
    m_transaction_start_count(0),
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
    m_questionnaireSerializer(nullptr),
    m_stmtGetFileOrderFromUuid(nullptr),
    m_stmtGetCaseRev(nullptr)
{
    ModifyCaseAccess(m_caseAccess);
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
    if (m_questionnaireSerializer)
        m_questionnaireSerializer->SetCaseAccess(m_caseAccess, IsReadOnly());
    if (m_db != nullptr) {
        ClearPreparedStatements();
        CreatePreparedStatements();
    }
}

void SQLiteRepository::Open(DataRepositoryOpenFlag open_flag)
{
    CString csDataFileName = WS2CS(m_connectionString.GetFilename());

    if (!SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(csDataFileName), GetFileExtension())) {
        CString csInvalidExtensionError;
        csInvalidExtensionError.Format(_T("The filename %s does not have the correct file extension. Must be \"%s\"."),
                                       PortableFunctions::PathGetDirectory(csDataFileName).c_str(), GetFileExtension());
        throw DataRepositoryException::IOError(csInvalidExtensionError);
    }

    bool bCanCreateFile = ( open_flag == DataRepositoryOpenFlag::CreateNew || open_flag == DataRepositoryOpenFlag::OpenOrCreate );

    if( bCanCreateFile && !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(csDataFileName)) )
    {
        CString csInvalidDirectoryError;
        csInvalidDirectoryError.Format(_T("The directory does not exist and could not be created: %s"),
                                       PortableFunctions::PathGetDirectory(csDataFileName).c_str());
        throw DataRepositoryException::IOError(csInvalidDirectoryError);
    }

    bool bFileExists = PortableFunctions::FileIsRegular(csDataFileName);

    if (!bFileExists && open_flag == DataRepositoryOpenFlag::OpenMustExist) {
        CString csMissingFileError;
        csMissingFileError.Format(_T("The data file does not exist: %s"), csDataFileName.GetString());
        throw DataRepositoryException::IOError(csMissingFileError);
    }

    // create a data file if one doesn't exist (or if clearing/opening in batch output mode, to overwrite what is already on the disk)
    if (!bFileExists || m_accessType == DataRepositoryAccess::BatchOutput || open_flag == DataRepositoryOpenFlag::CreateNew) {
        if (!CreateDatabaseFile()) {
            throw DataRepositoryException::IOError(_T("Could not create a new data file."));
        }
    } else {
        // Open the database
        OpenDatabaseFile();
    }

    if (GetSchemaVersion(m_db) <= 2)
        // Use blobs for backwards compatability
        m_questionnaireSerializer = new SQLiteBlobQuestionnaireSerializer(m_db);
    else
        // Relational format
        m_questionnaireSerializer = new SQLiteQuestionnaireSerializer(m_db);

    m_questionnaireSerializer->SetCaseAccess(m_caseAccess, IsReadOnly());

    CreatePreparedStatements();

    // For regular non-batch usage (main entry dict, external dict)
    // we will wrap every WriteCase (or group of WriteCases) in a transaction.
    // In batch mode all operations are wrapped in a few big transactions
    // to get decent performance for writes.
    if (!IsReadOnly() && m_accessType != DataRepositoryAccess::EntryInput && m_accessType != DataRepositoryAccess::ReadWrite) {
        StartTransaction();
    }

    m_transaction_file_revision = -1;
}

void SQLiteRepository::Close()
{
    if (m_db == NULL) {
        return;
    }

    ClearPreparedStatements();

    delete m_questionnaireSerializer;

    // For batch output create the indices at the end
    // This is faster for bulk writes
    if (m_accessType == DataRepositoryAccess::BatchOutput) {
        if (!CreateIndices(m_db))
            throw SQLiteErrorWithMessage(m_db);
    }

    EndTransaction();

    if (sqlite3_close(m_db) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    } else {
        m_db = NULL;
    }
}

const TCHAR* SQLiteRepository::GetFileExtension() const
{
    return FileExtensions::Data::CSProDB;
}

int SQLiteRepository::OpenSQLiteDatabaseFile(const ConnectionString& connection_string, sqlite3** ppDb, int flags)
{
    return sqlite3_open_v2(UTF8Convert::WideToUTF8(connection_string.GetFilename()).c_str(), ppDb, flags, nullptr);
}

int SQLiteRepository::OpenSQLiteDatabase(const ConnectionString& connection_string, sqlite3** ppDb, int flags)
{
    return OpenSQLiteDatabaseFile(connection_string, ppDb, flags);
}

bool SQLiteRepository::CreateDatabaseFile()
{
    CString csFilename = WS2CS(m_connectionString.GetFilename());

    if (PortableFunctions::FileExists(csFilename) && !PortableFunctions::FileDelete(csFilename))
        return false;

    sqlite3* pDB = NULL;

    if (OpenSQLiteDatabase(m_connectionString, &pDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) != SQLITE_OK)
        return false;

    // Default page size of 4096 is small. This improves performance with
    // bulk writes.
    sqlite3_exec(pDB, "PRAGMA page_size = 32768", NULL, NULL, NULL);

    if (m_accessType == DataRepositoryAccess::BatchOutput) {
        // These settings improve performance when doing lots of writes
        // to the database however if there is crash the database can become
        // corrupted. In batch output that is not a big deal since
        // we are creating the file from scratch each time.
        sqlite3_exec(pDB, "PRAGMA synchronous = OFF", NULL, NULL, NULL);
        sqlite3_exec(pDB, "PRAGMA journal_mode = OFF", NULL, NULL, NULL);

        // Since we create indices at the end in batch and foreign keys rely on indexes
        // we turn off foreign keys until the end after indexes are created
        sqlite3_exec(pDB, "PRAGMA foreign_keys = OFF;", NULL, NULL, NULL);
    }
    else {
        sqlite3_exec(pDB, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
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

    if (sqlite3_exec(pDB, create_static_tables_sql, NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(csFilename);
        return false;
    }

    // Don't create the indices for batch as it slows down database writes
    // We will create them at the end.
    if (m_accessType != DataRepositoryAccess::BatchOutput) {
        if (!CreateIndices(pDB)) {
            sqlite3_close(pDB);
            PortableFunctions::FileDelete(csFilename);
            return false;
        }
    }

    SQLiteDictionarySchemaGenerator schema_generator;
    std::ostringstream ss;
    ss << schema_generator.GenerateDictionary(m_caseAccess->GetDataDict());
    auto create_data_tables_sql = ss.str();

    //#define DUMP_SCHEMA 1
#if DUMP_SCHEMA
    std::ofstream schema_dump;
    auto dump_file_name = UTF8Convert::WideToUTF8(PortableFunctions::PathGetDirectory(csFilename) + PortableFunctions::PathGetFilenameWithoutExtension(csFilename) + "_schema.sql");
    schema_dump.open(dump_file_name);
    schema_dump
        << (create_static_tables_sql + strlen("BEGIN;"))
        << create_data_tables_sql;
    schema_dump.close();
#endif

    if (sqlite3_exec(pDB, create_data_tables_sql.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        const CString error = UTF8Convert::UTF8ToWide<CString>(sqlite3_errmsg(pDB));
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(csFilename);
        throw DataRepositoryException::SQLiteError(error);
    }

    const CDataDict& dictionary = m_caseAccess->GetDataDict();

    const char* insertVersions = "INSERT INTO meta(schema_version, cspro_version, dictionary, "
                                                  "dictionary_structure, dictionary_timestamp) "
                                                  "values(?,?,?,?,?);";
    if (SQLiteStatement(pDB, insertVersions)
        .Bind(1, SCHEMA_VERSION)
        .Bind(2, CSPRO_VERSION_NUMBER_DETAILED_TEXT)
        .Bind(3, dictionary.GetJson())
        .Bind(4, dictionary.GetStructureMd5())
        .Bind(5, dictionary.GetFileModifiedTime())
        .Step() != SQLITE_DONE) {
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(csFilename);
        return false;
    }

    if (sqlite3_exec(pDB, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_close(pDB);
        PortableFunctions::FileDelete(csFilename);
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

    if (sqlite3_prepare_v2(m_db, sql.str().c_str(), -1, &m_stmtUpdateCase, NULL) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }

    // Insert for use when case does not already exist
    sql.clear();
    sql.str("");
    sql <<
        "INSERT INTO cases(id, key, label, questionnaire, last_modified_revision, deleted, file_order, "
        "verified, partial_save_mode, partial_save_field_name, partial_save_level_key, partial_save_record_occurrence, partial_save_item_occurrence, partial_save_subitem_occurrence)"
        " VALUES(@id , @key , @dky, '', @rev , @del, COALESCE(@ord, (SELECT MAX(file_order) + 1 FROM cases), 1) , @ver , @psm , @psf , @psl , @psr , @psi , @pss)";

    if (sqlite3_prepare_v2(m_db, sql.str().c_str(), -1, &m_stmtInsertCase, NULL) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }

    if (sqlite3_prepare_v2(m_db, "SELECT file_order FROM cases WHERE id=?", -1, &m_stmtGetFileOrderFromUuid, NULL) != SQLITE_OK) {
        throw SQLiteErrorWithMessage(m_db);
    }

    if (sqlite3_prepare_v2(m_db, "SELECT id, file_order FROM cases WHERE deleted = 0 AND key=? LIMIT 1", -1, &m_stmtCaseIdentifiersFromKey, NULL) != SQLITE_OK) {
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
    safe_sqlite3_finalize(m_stmtGetCaseRev);
}

double SQLiteRepository::GetInsertPosition(double insert_before_position_in_repository)
{
    SQLiteStatement getPrevFileOrder(m_db, m_stmtGetPrevFileOrder, "SELECT file_order FROM cases WHERE file_order < ? ORDER BY file_order DESC LIMIT 1");
    getPrevFileOrder.Bind(1, insert_before_position_in_repository);

    double prevPos;
    if (getPrevFileOrder.Step() == SQLITE_ROW)
        prevPos = getPrevFileOrder.GetColumn<double>(0);
    else
        prevPos = 0; // before must be the first case in repo

    return (insert_before_position_in_repository + prevPos)/2;
}

std::unique_ptr<CDataDict> SQLiteRepository::ReadDictFromDatabase(sqlite3* pDB)
{
    SQLiteStatement getDictStatement(pDB, "SELECT dictionary FROM meta");
    if (getDictStatement.Step() != SQLITE_ROW) {
        sqlite3_close(pDB);
        throw DataRepositoryException::IOError(_T("Invalid file format. Missing dictionary."));
    }

    std::string dictContents = getDictStatement.GetColumn<std::string>(0);
    std::string_view dictContentsSV = dictContents;
    if (HasUtf8BOM(dictContents.data(), dictContents.size())) {
        dictContentsSV = dictContentsSV.substr(Utf8BOM_sv.length());
    }

    std::wstring dictContentsWide = UTF8Convert::UTF8ToWide(dictContentsSV);

    if (SO::IsWhitespace(dictContentsWide)) {
        throw DataRepositoryException::IOError(_T("Invalid file format. Dictionary is blank."));
    }

    try {
        auto dictionary = std::make_unique<CDataDict>();
        dictionary->OpenFromText(dictContentsWide);
        return dictionary;
    }
    catch (const CSProException& exception) {
        throw DataRepositoryException::IOError(FormatText(_T("Data file could not be read by this version of CSPro. ")
                                                          _T("It may have been created with a newer version of CSPro or it may be corrupt. ")
                                                          _T("Try opening it in the latest version of CSPro. %s"), exception.GetErrorMessage().c_str()));
    }
}

std::unique_ptr<CDataDict> SQLiteRepository::GetEmbeddedDictionary(const ConnectionString& connection_string)
{
    std::unique_ptr<CDataDict> dictionary;
    sqlite3* db = nullptr;

    if( OpenSQLiteDatabaseFile(connection_string, &db, SQLITE_OPEN_READONLY) == SQLITE_OK )
    {
        dictionary = ReadDictFromDatabase(db);
        sqlite3_close(db);
    }

    return dictionary;
}


void SQLiteRepository::OpenDatabaseFile()
{
    sqlite3* pDB = NULL;

    int flags = IsReadOnly() ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE;

    int result = OpenSQLiteDatabase(m_connectionString, &pDB, flags);
    if (result != SQLITE_OK) {
        throw DataRepositoryException::IOError(_T("Invalid file format. Not a valid database."));
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
        throw DataRepositoryException::IOError(_T("Data file was created by a newer version of CSPro and is not compatible with this version."));
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
        sqlite3_exec(pDB, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
}

void SQLiteRepository::DeleteRepository()
{
    Close();

    if( !PortableFunctions::FileDelete(m_connectionString.GetFilename()) )
        throw DataRepositoryException::DeleteRepositoryError();
}

bool SQLiteRepository::ContainsCase(const CString& key) const
{
    const char* containsCaseSql = "SELECT 1 FROM cases WHERE deleted = 0 AND key=? LIMIT 1";
    return SQLiteStatement(m_db, m_stmtContainsCase, containsCaseSql).
        Bind(1, key).
        Step() == SQLITE_ROW;
}

void SQLiteRepository::PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const
{
    auto bind_and_step = [](auto& statement, const auto& bind_value)
    {
        statement.Bind(1, bind_value);

        if (statement.Step() != SQLITE_ROW)
            throw DataRepositoryException::CaseNotFound();
    };

    if (!key.IsEmpty()) {
        SQLiteStatement getCaseIdentifiersFromKey(m_stmtCaseIdentifiersFromKey);
        bind_and_step(getCaseIdentifiersFromKey, key);
        uuid = getCaseIdentifiersFromKey.GetColumn<CString>(0);
        position_in_repository = getCaseIdentifiersFromKey.GetColumn<double>(1);
    }

    else if (!uuid.IsEmpty()) {
        SQLiteStatement getCaseIdentifiersFromUuid(m_db, m_stmtCaseIdentifiersFromUuid, "SELECT key, file_order FROM cases WHERE id=? LIMIT 1");
        bind_and_step(getCaseIdentifiersFromUuid, uuid);
        key = getCaseIdentifiersFromUuid.GetColumn<CString>(0);
        position_in_repository = getCaseIdentifiersFromUuid.GetColumn<double>(1);
    }

    else {
        SQLiteStatement getCaseIdentifiersFromFileOrder(m_db, m_stmtCaseIdentifiersFromFileOrder, "SELECT key, id FROM cases WHERE file_order=? LIMIT 1");
        bind_and_step(getCaseIdentifiersFromFileOrder, position_in_repository);
        key = getCaseIdentifiersFromFileOrder.GetColumn<CString>(0);
        uuid = getCaseIdentifiersFromFileOrder.GetColumn<CString>(1);
    }
}

std::optional<CaseKey> SQLiteRepository::FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
    const CaseIteratorParameters* start_parameters/* = nullptr*/) const
{
    auto statement = GetKeySearchIteratorStatement(0, 1, CaseIterationCaseStatus::NotDeletedOnly,
        iteration_method, iteration_order, start_parameters,
        _T("SELECT `cases`.`key`, `cases`.`file_order` "
           "FROM `cases` "
           "JOIN ( %s ) AS `filtered_cases` ON `cases`.`file_order` = `filtered_cases`.`file_order`"));

    std::optional<CaseKey> case_key;

    if( statement->Step() == SQLITE_ROW )
        case_key.emplace(statement->GetColumn<CString>(0), statement->GetColumn<double>(1));

    return case_key;
}

void SQLiteRepository::ReadCase(Case& data_case, const CString& key)
{
    SQLiteStatement getCaseByKey(m_stmtGetCaseByKey);
    getCaseByKey.Bind(1, key);

    if( !ReadCaseFromDatabase(data_case, getCaseByKey) )
        throw DataRepositoryException::CaseNotFound();
}

void SQLiteRepository::ReadCase(Case& data_case, double position_in_repository)
{
    SQLiteStatement getCaseByFileOrder(m_stmtGetCaseByFileOrder);
    getCaseByFileOrder.Bind(1, position_in_repository);

    if( !ReadCaseFromDatabase(data_case, getCaseByFileOrder) )
        throw DataRepositoryException::CaseNotFound();
}

bool SQLiteRepository::ReadCaseFromUuid(Case& data_case, CString uuid)
{
    SQLiteStatement getCaseById(m_stmtGetCaseById);
    getCaseById.Bind(1, uuid);
    return ReadCaseFromDatabase(data_case, getCaseById);
}

void SQLiteRepository::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/)
{
    bool new_case = false;

    switch (m_accessType) {
    case DataRepositoryAccess::BatchOutput:
    case DataRepositoryAccess::BatchOutputAppend:
    {
        ASSERT(write_case_parameter == NULL);

        // Preserve the uuid if there is one so that batch apps will keep
        // the same uuid in input and output files.

        // If there is no uuid (i.e. this comes from a text repo) then create one
        data_case.GetOrCreateUuid();

        data_case.SetPositionInRepository(0);

        // Batch is always treated as a new case
        // since we write into new output file
        // TODO: what about duplicates in batch append????
        new_case = true;
        break;
    }

    case DataRepositoryAccess::EntryInput:
    {
        // If there is no uuid (i.e. it is a new case) then create one
        data_case.GetOrCreateUuid();

        if (write_case_parameter == nullptr) {
            // New case - add to end of repo
            data_case.SetPositionInRepository(0);
            new_case = true;
        } else if (write_case_parameter->IsInsertParameter()) {
            double insert_before_position_in_repository = write_case_parameter->GetPositionInRepository();
            data_case.SetPositionInRepository(GetInsertPosition(insert_before_position_in_repository));
            new_case = true;
        } else {
            ASSERT(write_case_parameter->IsModifyParameter());
            new_case = false;
        }
        break;
    }

    case DataRepositoryAccess::ReadWrite:
    {
        ASSERT(write_case_parameter == NULL);

        // From writecase we update the case based on the case id (key)
        // and not the uuid. This way logic like:
        //      ID = 1; writecase(DICT); ID = 2; writecase(DICT)
        // will generate two distinct cases even though it ends
        // up being two consecutive calls to WriteCase() with
        // the same uuid.
        SQLiteStatement getUuidPosFromKey(m_stmtCaseIdentifiersFromKey);
        getUuidPosFromKey.Bind(1, data_case.GetKey());

        int queryResult = getUuidPosFromKey.Step();
        if (queryResult == SQLITE_ROW) {
            // There is already a case with this case id,
            // use the same uuid to overwrite it
            data_case.SetUuid(getUuidPosFromKey.GetColumn<std::wstring>(0));
            data_case.SetPositionInRepository(getUuidPosFromKey.GetColumn<double>(1));
            new_case = false;
        } else if (queryResult == SQLITE_DONE) {
            // No existing case with this case id so
            // create a new uuid to generate a new case
            data_case.SetUuid(CreateUuid());
            data_case.SetPositionInRepository(0);
            new_case = true;
        } else
            throw SQLiteErrorWithMessage(m_db);

        break;
    }

    case DataRepositoryAccess::BatchInput:
    case DataRepositoryAccess::ReadOnly:
        throw DataRepositoryException::WriteAccessRequired();
    }

    int64_t revision;

    if (m_transaction_start_count > 0) {

        // Only start a new revision if we haven't added
        // one since we opened the file.
        if (m_transaction_file_revision == -1) {
            m_transaction_file_revision = AddFileRevision();
        }
        revision = m_transaction_file_revision;

    } else {

        // Start a new revision for each write (in batch we use a single revision)
        revision = AddFileRevision();
    }

    StartTransaction();

    if (new_case) {
        int insertResult = InsertCase(data_case, revision);
        if (insertResult == SQLITE_CONSTRAINT && m_accessType == DataRepositoryAccess::BatchOutputAppend) {
            // Duplicate uuid in batch append mode, create a new uuid to allow duplicate
            data_case.SetUuid(CreateUuid());
            insertResult = InsertCase(data_case, revision);

            if( data_case.GetCaseConstructionReporter() != nullptr )
                data_case.GetCaseConstructionReporter()->DuplicateUuid(data_case);
        }
        if (insertResult != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }

        // Update the file pos in the case to make caching work
        if (m_accessType == DataRepositoryAccess::ReadWrite)
            UpdateFilePosition(data_case);

        data_case.GetVectorClock().increment(m_deviceId);
        InsertVectorClock(data_case);
        WriteNotes(data_case);
    } else {
        if (UpdateCase(data_case, revision) != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }
        IncrementVectorClock(data_case.GetUuid());
        if (write_case_parameter == nullptr || !write_case_parameter->IsModifyParameter() || write_case_parameter->AreNotesModified()) {
            ClearNotes(data_case);
            WriteNotes(data_case);
        }
    }

    EndTransaction();
    CommitTransactionIfTooBig();
}

void SQLiteRepository::CommitTransactionIfTooBig()
{
    const int MaxNumberSqlInsertsInOneTransaction = 2500;
    if (++m_iInsertInTransactionCounter == MaxNumberSqlInsertsInOneTransaction) {
        sqlite3_exec(m_db, "COMMIT", NULL, NULL, NULL);
        sqlite3_exec(m_db, "BEGIN", NULL, NULL, NULL);
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

void SQLiteRepository::IncrementVectorClock(CString uuid)
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

void SQLiteRepository::IncrementVectorClock(double position_in_repository)
{
    CString key;
    CString uuid;
    PopulateCaseIdentifiers(key, uuid, position_in_repository);

    IncrementVectorClock(uuid);
}

void SQLiteRepository::DeleteCase(double position_in_repository, bool deleted/* = true*/)
{
    StartTransaction();

    int64_t revision;

    if (m_transaction_start_count) {

        // Only start a new revision if we haven't added
        // one since we opened the file.
        if (m_transaction_file_revision == -1) {
            m_transaction_file_revision = AddFileRevision();
        }
        revision = m_transaction_file_revision;

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

    IncrementVectorClock(position_in_repository);

    EndTransaction();
    CommitTransactionIfTooBig();
}

size_t SQLiteRepository::GetNumberCases() const
{
    SQLiteStatement statement(m_db, m_stmtCountCases, "SELECT COUNT(*) FROM cases WHERE deleted = 0");

    if( statement.Step() == SQLITE_ROW )
        return statement.GetColumn<size_t>(0);

    throw DataRepositoryException::SQLiteError();
}

size_t SQLiteRepository::GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters/* = nullptr*/) const
{
    if( case_status == CaseIterationCaseStatus::NotDeletedOnly && start_parameters == nullptr )
        return GetNumberCases();

    auto statement = GetKeySearchIteratorStatement(0, SIZE_MAX, case_status, std::nullopt, std::nullopt, start_parameters,
        _T("SELECT COUNT(*) "
           "FROM `cases` "
           "JOIN ( %s ) AS `filtered_cases` ON `cases`.`file_order` = `filtered_cases`.`file_order`"));

    if( statement->Step() == SQLITE_ROW )
        return statement->GetColumn<size_t>(0);

    throw DataRepositoryException::SQLiteError();
}

void SQLiteRepository::WriteIteratorSelectFromSql(std::wstringstream& sql, CaseIterationContent iteration_content) const
{
    bool get_case_note_for_case_summary = ( iteration_content == CaseIterationContent::CaseSummary &&
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

std::unique_ptr<CaseIterator> SQLiteRepository::CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
    std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
    const CaseIteratorParameters* start_parameters/* = nullptr*/, size_t offset/* = 0*/, size_t limit/* = SIZE_MAX*/)
{
    bool get_case_note_for_case_summary = ( iteration_content == CaseIterationContent::CaseSummary &&
                                            m_caseAccess->GetUsesNotes() && CaseIterator::RequiresCaseNote() );

    std::wstringstream sql;
    WriteIteratorSelectFromSql(sql, iteration_content);
    sql << "JOIN ( %s ) AS `filtered_cases` ON `cases`.`file_order` = `filtered_cases`.`file_order` ";

    if( get_case_note_for_case_summary )
    {
        sql << "LEFT OUTER JOIN `notes` ON `cases`.`id` = `notes`.`case_id` AND "
               "`notes`.`field_name` = '" << m_caseAccess->GetDataDict().GetName().GetString() << "' AND `notes`.`operator_id`='' ";
    }

    return std::make_unique<SQLiteRepositoryCaseIterator>(*this, iteration_content,
        GetKeySearchIteratorStatement(offset, limit, case_status, iteration_method, iteration_order, start_parameters, sql.str().c_str()),
        case_status, start_parameters);
}

std::unique_ptr<SQLiteStatement> SQLiteRepository::GetKeySearchIteratorStatement(size_t offset, size_t limit,
    CaseIterationCaseStatus case_status, std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
    const CaseIteratorParameters* start_parameters, const TCHAR* base_sql) const
{
    CString order_by_text;

    if( iteration_method.has_value() )
    {
        order_by_text.Format(_T("ORDER BY %s %s "),
            ( iteration_method == CaseIterationMethod::KeyOrder ) ? _T("`cases`.`key`") : _T("`cases`.`file_order`"),
            ( ( iteration_order == CaseIterationOrder::Ascending )  ? _T("ASC") :
              ( iteration_order == CaseIterationOrder::Descending ) ? _T("DESC") :
                                                                      _T("") ));
    }

    CString limit_text = FormatText(_T("LIMIT %d OFFSET %d "), ( limit == SIZE_MAX ) ? -1 : (int)limit, (int)offset);

    CString where_text;

    auto add_to_where_text = [&](const TCHAR* condition)
    {
        where_text.AppendFormat(_T("%s ( %s ) "), where_text.IsEmpty() ? _T("WHERE") : _T("AND"), condition);
    };

    // process any filters
    bool use_key_prefix = false;
    bool use_operators = false;

    if( start_parameters != nullptr )
    {
        // use the key prefix if it is set and and is not empty
        if( start_parameters->key_prefix.has_value() && !start_parameters->key_prefix->IsEmpty() )
        {
            use_key_prefix = true;
            where_text = _T("WHERE `cases`.`key` >= ? AND `cases`.`key` < ?");

            use_operators = std::holds_alternative<CString>(start_parameters->first_key_or_position) ?
                !std::get<CString>(start_parameters->first_key_or_position).IsEmpty() :
                ( std::get<double>(start_parameters->first_key_or_position) != -1 );
        }

        else
        {
            use_operators = true;
        }

        if( use_operators )
        {
            const TCHAR* comparison_operator =
                ( start_parameters->start_type == CaseIterationStartType::LessThan )          ?   _T("<") :
                ( start_parameters->start_type == CaseIterationStartType::LessThanEquals )    ?   _T("<=") :
                ( start_parameters->start_type == CaseIterationStartType::GreaterThanEquals ) ?   _T(">=") :
              /*( start_parameters->start_type == CaseIterationStartType::GreaterThan )       ?*/ _T(">");

            add_to_where_text(FormatText(_T("%s %s ?"), std::holds_alternative<CString>(start_parameters->first_key_or_position) ?
                _T("`cases`.`key`") : _T("`cases`.`file_order`"), comparison_operator));
        }
    }

    // filter on case properties
    if( case_status != CaseIterationCaseStatus::All )
    {
        add_to_where_text(_T("`cases`.`deleted` = 0"));

        if( case_status == CaseIterationCaseStatus::PartialsOnly )
        {
            add_to_where_text(_T("`cases`.`partial_save_mode` IS NOT NULL"));
        }

        else if( case_status == CaseIterationCaseStatus::DuplicatesOnly )
        {
            add_to_where_text(_T("`cases`.`key` IN ( SELECT `cases`.`key` FROM `cases` WHERE `cases`.`deleted` = 0 GROUP BY `cases`.`key` HAVING COUNT(*) > 1 )"));
        }
    }

    // generate the complete SQL statement
    CString filter_sql = FormatText(_T("SELECT `cases`.`file_order` FROM `cases` %s %s %s "), where_text.GetString(), order_by_text.GetString(), limit_text.GetString());

    CString sql;
    sql.Format(base_sql, filter_sql.GetString());
    sql.AppendFormat(order_by_text);

    auto statement = std::make_unique<SQLiteStatement>(m_db, sql);

    if( use_key_prefix )
    {
        std::string key_prefix = ToUtf8(*start_parameters->key_prefix);
        statement->Bind(1, key_prefix);
        statement->Bind(2, SQLiteHelpers::GetTextPrefixBoundary(key_prefix));
    }

    if( use_operators )
    {
        int operator_argument_index = use_key_prefix ? 3 : 1;

        if( std::holds_alternative<CString>(start_parameters->first_key_or_position) )
        {
            statement->Bind(operator_argument_index, std::get<CString>(start_parameters->first_key_or_position));
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
    std::wstring embedded_dict_sig = GetDictionaryStructureMd5(*pDB);
    if (!embedded_dict_sig.empty() && embedded_dict_sig == m_caseAccess->GetDataDict().GetStructureMd5())
        return;

    std::unique_ptr<CDataDict> pOriginalDict = ReadDictFromDatabase(*pDB);
    SQLiteDictionarySchemaReconciler reconciler(*pDB, *pOriginalDict, m_caseAccess.get());
    if (reconciler.NeedsReconcile()) {
        MakeDatabaseTemporarilyWriteable(pDB);
        reconciler.Reconcile(*pDB);
        EndMakeDatabaseTemporarilyWriteable(pDB);
    }
}

std::wstring SQLiteRepository::GetDictionaryStructureMd5(sqlite3* db)
{
    SQLiteStatement get_structure_statement(db, "SELECT dictionary_structure FROM meta");

    // this column only exists starting starting with CSPro 7.5
    if (get_structure_statement.Step() != SQLITE_ROW) {
        return std::wstring();
    }
    else {
        return get_structure_statement.GetColumn<std::wstring>(0);
    }
}

int SQLiteRepository::GetSchemaVersion(sqlite3 * pDB) const
{
    SQLiteStatement getMetaStatement(pDB, "SELECT schema_version FROM meta");
    if (getMetaStatement.Step() != SQLITE_ROW) {
        // The meta table could be missing if this isn't really a csdb file but also could be
        // if the file is currently open as output to batch edit program which has deleted
        // and recreated file but has not yet flushed transaction that creates the meta table.
        throw DataRepositoryException::IOError(_T("Unable to read CSPro DB file. Could be an invalid file, a corrupt file or it could be in use by another program."));
    }

    return getMetaStatement.GetColumn<int>(0);
}

void SQLiteRepository::MigrateFromSchemaVersion1(sqlite3* pDB)
{
    const char *sql = "BEGIN TRANSACTION;"
        "ALTER TABLE file_revisions RENAME TO temp_file_revisions;"
        "CREATE TABLE file_revisions("
        "	id INTEGER NOT NULL PRIMARY KEY,"
        "	device_id TEXT NOT NULL,"
        "	timestamp INTEGER NOT NULL default (strftime('%s', 'now')));"
        "INSERT INTO file_revisions"
        "	SELECT"
        "	id, device_id, timestamp"
        "	FROM"
        "	temp_file_revisions;"
        "CREATE TABLE sync_history("
        "	id INTEGER NOT NULL PRIMARY KEY,"
        "	file_revision INTEGER NOT NULL,"
        "   device_id TEXT NOT NULL,"
        "   timestamp INTEGER NOT NULL default (strftime('%s','now')),"
        "	universe TEXT NULL,"
        "	direction INTEGER NULL,"
        "	server_revision TEXT NULL,"
        "	partial INTEGER default 0,"
        "	last_id TEXT NULL default NULL,"
        "	FOREIGN KEY(file_revision) REFERENCES file_revisions(id));"
        "DROP TABLE temp_file_revisions;"
        "UPDATE meta SET schema_version=2;"
        "COMMIT;";

    int rc = sqlite3_exec(pDB, sql, NULL, NULL, NULL);
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
        "	SELECT"
        "	id, file_revision, device_id, device_id, null, timestamp, universe, direction, server_revision, partial, last_id"
        "	FROM"
        "	temp_sync_history;"
        "DROP TABLE temp_sync_history;"
        "COMMIT;";

    int rc = sqlite3_exec(pDB, sql, NULL, NULL, NULL);
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
    //if the binary-sync-history table is missing then the arhive table is also missing 
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
            throw DataRepositoryException::IOError(_T("Failed to reopen database as writeable."));
        }
    }
}

void SQLiteRepository::EndMakeDatabaseTemporarilyWriteable(sqlite3** db)
{
    if (IsReadOnly()) {
        sqlite3_close(*db);
        if (OpenSQLiteDatabase(m_connectionString, db, SQLITE_OPEN_READWRITE) != SQLITE_OK) {
            throw DataRepositoryException::IOError(_T("Failed to reopen database after making writeable."));
        }
    }
}

void SQLiteRepository::StartSync(DeviceId remoteDeviceId, CString remoteDeviceName, CString userName, SyncDirection direction, CString universe, bool bUpdateOnConflict)
{
    if (m_accessType == DataRepositoryAccess::BatchInput) {
        throw DataRepositoryException::WriteAccessRequired();
    }

    if (m_accessType == DataRepositoryAccess::BatchOutput || m_accessType == DataRepositoryAccess::BatchOutputAppend) {
        throw DataRepositoryException::NotValidInBatchOutput();
    }

    if (m_accessType == DataRepositoryAccess::ReadOnly) {
        //fix to allow readonly external file to be synced by reopening in read write mode

        try {
            //to resolve sqlite_busy issues when closing the file add a handler that waits for max of 5 seconds before erroring on close
            sqlite3_busy_timeout(m_db, 5000);
            Close();
            //reopen the file in readwrite mode to allow sync writes
            m_accessType = DataRepositoryAccess::ReadWrite;
            m_caseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(m_caseAccess->GetDataDict());
            Open(DataRepositoryOpenFlag::OpenMustExist);
        }
        catch (...) {
            throw DataRepositoryException::IOError(_T("Unable to close and reopen file read-write to sync previously read-only file."));
        }
    }

    m_currentSyncStats = { 0 };

    // Save these here - they get written to DB when cases are received/sent that way
    // we don't record anything if we never contact the server.
    m_currentSyncParams.m_currentSyncId = 0; // Gets set later when we write to DB
    m_currentSyncParams.m_currentFileRevision = 0; // Gets set later when we write to DB
    m_currentSyncParams.m_remoteDeviceId = remoteDeviceId;
    m_currentSyncParams.m_remoteDeviceName = remoteDeviceName;
    m_currentSyncParams.m_userName = userName;
    m_currentSyncParams.m_direction = direction;
    m_currentSyncParams.m_universe = universe;
    m_currentSyncParams.m_currentSyncUpdateOnConflict = bUpdateOnConflict;
    m_currentSyncParams.m_serverRevision.Empty();
}

int SQLiteRepository::SyncCasesFromRemote(const std::vector<std::shared_ptr<Case>>& cases_received, CString serverRevision)
{
    m_currentSyncParams.m_serverRevision = serverRevision;

    if (m_currentSyncParams.m_currentSyncId == 0) {
        // First cases received this sync, haven't added rev to database yet
        if (m_transaction_start_count) {
            if (m_transaction_file_revision == -1) {
                m_transaction_file_revision = AddFileRevision();
            }
            m_currentSyncParams.m_currentFileRevision = m_transaction_file_revision;
        }
        else {
            m_currentSyncParams.m_currentFileRevision = AddFileRevision();
        }
        m_currentSyncParams.m_currentSyncId =
            AddSyncHistoryEntry(m_currentSyncParams.m_currentFileRevision,
                m_currentSyncParams.m_remoteDeviceId, m_currentSyncParams.m_remoteDeviceName,
                m_currentSyncParams.m_userName,
                m_currentSyncParams.m_direction, m_currentSyncParams.m_universe,
                serverRevision, SyncHistoryEntry::SyncState::PartialGet,
                CString());
    }

    Case local_case(m_caseAccess->GetCaseMetadata());

    for( const std::shared_ptr<Case>& remote_case : cases_received ) {

        ++m_currentSyncStats.numReceived;

        if (ReadCaseFromUuid(local_case, remote_case->GetUuid())) {
            // Case exists both local and remote, compare using vector clocks
            if (remote_case->GetVectorClock() < local_case.GetVectorClock()) {
                // Local case is more recent, do not update
                ++m_currentSyncStats.numCasesNewerInRepo;
            } else if (local_case.GetVectorClock() < remote_case->GetVectorClock()) {
                // Update is newer, replace the local case
                const bool bNewCase = false;
                SyncCase(*remote_case, m_currentSyncParams.m_currentFileRevision, bNewCase, m_currentSyncParams.m_currentSyncId);
                ++m_currentSyncStats.numCasesNewerOnRemote;
            } else if (local_case.GetVectorClock() != remote_case->GetVectorClock()) {
                // Conflict - neither clock is greater
                ++m_currentSyncStats.numConflicts;

                // Make a new case with merged clocks
                VectorClock mergedClock = local_case.GetVectorClock();
                mergedClock.merge(remote_case->GetVectorClock());

                // Use the data from the remote case if we update on conflict otherwise use data from local
                // The updateOnConflict will be true on the server and false on the client so that the client
                // always wins conflicts.
                Case& baseCaseForResolve = m_currentSyncParams.m_currentSyncUpdateOnConflict ? *remote_case : local_case;
                VectorClock baseCaseClock = baseCaseForResolve.GetVectorClock();
                baseCaseForResolve.SetVectorClock(mergedClock);
                const bool bNewCase = false;
                SyncCase(baseCaseForResolve, m_currentSyncParams.m_currentFileRevision, bNewCase, m_currentSyncParams.m_currentSyncId);
                baseCaseForResolve.SetVectorClock(baseCaseClock);
            }
        } else {
            // No local version of the case so it must be new, add it to repo
            ++m_currentSyncStats.numNewCasesNotInRepo;

            const bool bNewCase = true;
            SyncCase(*remote_case, m_currentSyncParams.m_currentFileRevision, bNewCase, m_currentSyncParams.m_currentSyncId);
        }
    }

    m_currentSyncParams.m_serverRevision = serverRevision;
    SetSyncRevisionPartial(m_currentSyncParams.m_currentSyncId,
        SyncHistoryEntry::SyncState::PartialGet,
        serverRevision,
        cases_received.empty() ? CString() : cases_received.back()->GetUuid(),
        m_currentSyncParams.m_currentFileRevision);

    return m_currentSyncParams.m_currentFileRevision;
}

void SQLiteRepository::MarkCasesSentToRemote(const std::vector<std::shared_ptr<Case>>& cases_sent, std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryCaseItems, CString serverRevision, int clientRevision)
{
    m_currentSyncParams.m_currentFileRevision = clientRevision;
    m_currentSyncParams.m_serverRevision = serverRevision;

    if (m_currentSyncParams.m_currentSyncId == 0) {

        // First cases received this sync, haven't added rev to database yet
        m_currentSyncParams.m_currentSyncId =
            AddSyncHistoryEntry(m_currentSyncParams.m_currentFileRevision,
                m_currentSyncParams.m_remoteDeviceId, m_currentSyncParams.m_remoteDeviceName,
                m_currentSyncParams.m_userName,
                m_currentSyncParams.m_direction, m_currentSyncParams.m_universe,
                m_currentSyncParams.m_serverRevision, SyncHistoryEntry::SyncState::PartialPut,
                CString());
    }

    CString lastUuid = cases_sent.empty() ? CString() : cases_sent.back()->GetUuid();

    SetSyncRevisionPartial(m_currentSyncParams.m_currentSyncId, SyncHistoryEntry::SyncState::PartialPut,
        m_currentSyncParams.m_serverRevision, lastUuid, m_currentSyncParams.m_currentFileRevision);

    m_currentSyncStats.numSent += cases_sent.size();

    AddBinaryItemsSyncHistory(binaryCaseItems, m_currentSyncParams.m_currentSyncId);
}


namespace
{
    inline void AddBinaryItemsSyncHistoryWorker(std::set<std::wstring>& synced_signatures, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
    {
        const BinaryDataMetadata* binary_data_metadata = binary_case_item.GetBinaryDataMetadata_noexcept(index);

        if( binary_data_metadata == nullptr )
        {
            ASSERT(false);
            return;
        }

        const SyncJsonBinaryDataReader* sync_json_binary_data_reader = dynamic_cast<const SyncJsonBinaryDataReader*>(binary_case_item.GetBinaryDataAccessor(index).GetBinaryDataReader());

        // only add entries once per file per sync (so if multiple cases share the same file, the binary-sync-history table will only have one entry per sync)
        if( sync_json_binary_data_reader != nullptr && !sync_json_binary_data_reader->HasSyncedContent() )
            return;

        synced_signatures.insert(binary_data_metadata->GetBinaryDataKey());
    }
}


void SQLiteRepository::AddBinaryItemsSyncHistory(const Case& data_case, int syncHistoryId)
{
    if( !data_case.GetCaseMetadata().UsesBinaryData() )
        return;

    std::set<std::wstring> synced_signatures;

    data_case.ForeachDefinedBinaryCaseItem(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            AddBinaryItemsSyncHistoryWorker(synced_signatures, binary_case_item, index);
        });

    AddBinaryItemsSyncHistory(synced_signatures, syncHistoryId);
}


void SQLiteRepository::AddBinaryItemsSyncHistory(const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryCaseItems, int syncHistoryId)
{
    if( binaryCaseItems.empty() )
        return;

    std::set<std::wstring> synced_signatures;

    for( const auto& [binary_case_item, index] : binaryCaseItems )
        AddBinaryItemsSyncHistoryWorker(synced_signatures, *binary_case_item, index);

    AddBinaryItemsSyncHistory(synced_signatures, syncHistoryId);
}


void SQLiteRepository::AddBinaryItemsSyncHistory(const std::set<std::wstring>& synced_signatures, int syncHistoryId)
{
    if( synced_signatures.empty() )
        return;

    // Insert into binary-sync-history
    SQLiteStatement insertBinarySyncHistoryStatement(m_db, m_stmtInsertBinarySyncHistory,
        "INSERT INTO `binary-sync-history`(`binary-data-signature`,`sync-history-id`)"
        "VALUES( @signature, @id)");

     for( const std::wstring& signature : synced_signatures )
     {
         ASSERT(!signature.empty());

         insertBinarySyncHistoryStatement
             .Bind("@signature", signature)
             .Bind("@id", syncHistoryId);

         if( insertBinarySyncHistoryStatement.Step() != SQLITE_DONE )
            throw SQLiteErrorWithMessage(m_db);

         insertBinarySyncHistoryStatement.Reset();
    }
}


void SQLiteRepository::ClearBinarySyncHistory(CString serverDeviceId, int fileRevision /* = -1*/)
{
    //This function is called when sync client cannot find a revision on the server and has to do a full sync 
    //This should be only for binary items that have been synced to this / from server

    //Archive the binary sync history when its a full resync by moving any syncs including the binary items associated to gets as well
    // as the server may have deleted all the binary items and we need to resend them
    //all our binaries in this case again as we cannot find the last revision on the server
    std::ostringstream sql;
    sql << "INSERT INTO `binary-sync-history-archive` (`binary-sync-history-id`, `binary-data-signature`, `sync-history-id`)"
           " SELECT `binary-sync-history`.`id`, `binary-data-signature`,"
           " `sync-history-id` FROM `binary-sync-history` JOIN `sync_history` ON `binary-sync-history`.`sync-history-id` = `sync_history`.`id`"
           " WHERE `sync_history`.`device_id` = @serverDeviceId AND `sync_history`.`file_revision` >= @fileRevision" ;

    SQLiteStatement archiveInvalidBinarySyncHistoryEntries(m_db, m_stmtArchiveBinarySyncHistory, sql.str().c_str());
    archiveInvalidBinarySyncHistoryEntries.Bind("@serverDeviceId", serverDeviceId);
    archiveInvalidBinarySyncHistoryEntries.Bind("@fileRevision", fileRevision);
    if (archiveInvalidBinarySyncHistoryEntries.Step() != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);

    sql.clear();
    sql.str("");
    //Clear the binary sync history when its a full resync by deleting any syncs as we need  to resend
    //all our binaries in this case again as we cannot find the last revision on the server
    sql << "DELETE FROM `binary-sync-history` WHERE `binary-sync-history`.`id` IN(SELECT `binary-sync-history`.`id` FROM `binary-sync-history` JOIN `sync_history` ON `binary-sync-history`.`sync-history-id` = `sync_history`.`id`";
    sql << " WHERE `sync_history`.`device_id` = @serverDeviceId AND `sync_history`.`file_revision` >= @fileRevision)";
    SQLiteStatement deleteInvalidBinarySyncHistoryEntries(m_db, m_stmtDeleteBinarySyncHistory, sql.str().c_str());
    deleteInvalidBinarySyncHistoryEntries.Bind("@serverDeviceId", serverDeviceId);
    deleteInvalidBinarySyncHistoryEntries.Bind("@fileRevision", fileRevision);
    if (deleteInvalidBinarySyncHistoryEntries.Step() != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);
}

void SQLiteRepository::EndSync()
{
    SetSyncRevisionComplete(m_currentSyncParams.m_currentSyncId);
}

ISyncableDataRepository::SyncStats SQLiteRepository::GetLastSyncStats() const
{
    return m_currentSyncStats;
}

void SQLiteRepository::SyncCase(Case& data_case, int fileRevision, bool bNewCase, int currentSyncId)
{
    // Ensure that position is not copied from remote repo
    data_case.SetPositionInRepository(0);

    if (bNewCase) {
        if (InsertCase(data_case, fileRevision) != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }
        InsertVectorClock(data_case);
    } else {
        if (UpdateCase(data_case, fileRevision) != SQLITE_DONE) {
            throw SQLiteErrorWithMessage(m_db);
        }
        UpdateVectorClock(data_case);
        ClearNotes(data_case);
    }

    WriteNotes(data_case);
    AddBinaryItemsSyncHistory(data_case, currentSyncId);
}

void SQLiteRepository::InsertVectorClock(const Case& data_case)
{
    for ( const CString& device : data_case.GetVectorClock().getAllDevices() ) {
        SQLiteStatement updateClockStatement(m_db, m_stmtNewClock,
            "INSERT INTO vector_clock(case_id, device, revision)"
            "VALUES(@id , @dev , @rev)");
        updateClockStatement
            .Bind("@id", data_case.GetUuid())
            .Bind("@dev", device)
            .Bind("@rev", data_case.GetVectorClock().getVersion(device));

        if (updateClockStatement.Step() != SQLITE_DONE)
            throw SQLiteErrorWithMessage(m_db);
    }
}

void SQLiteRepository::UpdateVectorClock(const Case& data_case)
{
    for ( const CString& device : data_case.GetVectorClock().getAllDevices() ) {
        SQLiteStatement updateClockStatement(m_db, m_stmtUpdateClock,
            "INSERT OR REPLACE INTO vector_clock(case_id, device, revision)"
            "VALUES(@id , @dev , @rev)");
        updateClockStatement
            .Bind("@id", data_case.GetUuid())
            .Bind("@dev", device)
            .Bind("@rev", data_case.GetVectorClock().getVersion(device));

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

    auto case_result = insertOrUpdateCase.Step();
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

std::unique_ptr<CaseIterator> SQLiteRepository::GetCasesModifiedSinceRevisionIterator(int fileRevision, CString lastUuid,
    CString universe, int limit /*=INT_MAX*/, int* pCaseCount /* = 0 */,
    int *pLastFileRev /* = 0 */, CString ignoreGetsFromDeviceId /*= CString()*/,
    const std::vector<std::wstring>& ignoreRevisions /*= std::vector<std::wstring>()*/)
{
    std::wstringstream where_sql;
    where_sql <<
        "(last_modified_revision > @rev OR ( @lid IS NOT NULL AND @lid != '' AND last_modified_revision = @rev AND id > @lid )) ";
    if (!universe.IsEmpty())
        where_sql << "AND key LIKE @uni ";
    if (!ignoreGetsFromDeviceId.IsEmpty())
        where_sql << "AND last_modified_revision NOT IN (SELECT file_revision FROM sync_history WHERE device_id=@dev AND direction = 2) ";
    if (!ignoreRevisions.empty()) {
        where_sql << "AND last_modified_revision NOT IN (";
        for (size_t i = 0; i < ignoreRevisions.size(); ++i) {
            if (i != 0)
                where_sql << ',';
            where_sql << "@ir" << i;
        }
        where_sql << ") ";
    }

    if (pCaseCount) {
        std::wstringstream sql;
        sql << "SELECT COUNT(*) FROM cases WHERE "
            << where_sql.str();
        SQLiteStatement countStmt(m_db, sql.str().c_str());
        countStmt.Bind("@rev", fileRevision).Bind("@lid", lastUuid);
        if (!universe.IsEmpty())
            countStmt.Bind("@uni", universe + _T("%"));
        if (!ignoreGetsFromDeviceId.IsEmpty())
            countStmt.Bind("@dev", ignoreGetsFromDeviceId);
        if (!ignoreRevisions.empty()) {
            for (size_t i = 0; i < ignoreRevisions.size(); ++i) {
                std::ostringstream paramName;
                paramName << "@ir" << i;
                countStmt.Bind(paramName.str().c_str(), ignoreRevisions[i]);
            }
        }
        countStmt.Step();
        *pCaseCount = countStmt.GetColumn<int>(0);
    }

    if (pLastFileRev) {
        std::wstringstream sql;
        sql << "SELECT COALESCE(MAX(last_modified_revision), (SELECT MAX(last_modified_revision) FROM cases)) "
            << "FROM (SELECT last_modified_revision FROM cases WHERE "
            << where_sql.str()
            << "ORDER BY last_modified_revision "
            << "LIMIT @lim)";

        SQLiteStatement maxStmt(m_db, sql.str().c_str());
        maxStmt.Bind("@rev", fileRevision).
            Bind("@lid", lastUuid).
            Bind("@lim", limit);
        if (!universe.IsEmpty())
            maxStmt.Bind("@uni", universe + _T("%"));
        if (!ignoreGetsFromDeviceId.IsEmpty())
            maxStmt.Bind("@dev", ignoreGetsFromDeviceId);
        if (!ignoreRevisions.empty()) {
            for (size_t i = 0; i < ignoreRevisions.size(); ++i) {
                std::ostringstream paramName;
                paramName << "@ir" << i;
                maxStmt.Bind(paramName.str().c_str(), ignoreRevisions[i]);
            }
        }
        maxStmt.Step();
        *pLastFileRev = maxStmt.GetColumn<int>(0);
    }

    std::wstringstream sql;
    WriteIteratorSelectFromSql(sql, CaseIterationContent::Case);
    sql << "WHERE " << where_sql.str().c_str()
        << "ORDER BY last_modified_revision, id "
           "LIMIT @lim";

    auto statement = std::make_unique<SQLiteStatement>(m_db, sql.str().c_str());
    statement->Bind("@rev", fileRevision)
              .Bind("@lid", lastUuid)
              .Bind("@lim", limit);
    if (!universe.IsEmpty())
        statement->Bind("@uni", universe + _T("%"));
    if (!ignoreGetsFromDeviceId.IsEmpty())
        statement->Bind("@dev", ignoreGetsFromDeviceId);
    if (!ignoreRevisions.empty()) {
        for (size_t i = 0; i < ignoreRevisions.size(); ++i) {
            std::ostringstream paramName;
            paramName << "@ir" << i;
            statement->Bind(paramName.str().c_str(), ignoreRevisions[i]);
        }
    }

    return std::make_unique<SQLiteRepositoryCaseIterator>(*this, CaseIterationContent::Case, std::move(statement));
}


std::set<std::wstring> SQLiteRepository::GetBinarySignaturesModifiedSinceRevision(const CString& caseUuid, std::set<std::string>& md5ExcludeKeys, DeviceId deviceId)
{
    /*determine which binary items need to be uploaded in the next sync upload we just need to find all the binary items that do NOT have a corresponding
    entry in the binary-sync-history entry that matches or exceeds the last_modified_revision of the item. This means left joining cases, case-binary-data,
    binary-data, binary-sync-history and file-revisions filtered by the universe and the server (to ignore syncs with other servers), and then grouping the
    results by binary-data.id, taking the max(sync_history.file_revision) and filtering the results to keep only
    those rows where max(sync_history.file_revision) is null or less than binary-data.file_revision. */

    //Get all the binary signatures for this case that have not been synced to the given device since the last modified revision
    //if the binary itemes have never been synced to the given device return all the md5s for the case
    std::set<std::wstring> md5Signatures;

    std::wstringstream whereClause;
    if (!deviceId.IsEmpty()) {
        //select binary items that have either never been sent or sent to this server and check if the max file revision sent is less than
        //the current version of the binary item and ensure that it is sent
        //To avoid sending binary items we got from this server - ignore binary items from this server where the file version have not changed since
        whereClause << " WHERE last_modified_revision NOT IN (SELECT file_revision FROM sync_history WHERE device_id=@dev AND direction = 2) ";
    }
    
    std::wstringstream sql;
    sql << " SELECT signature, MAX(file_revision) AS max_synced_file_revision, last_modified_revision "
        << " FROM  (SELECT `case-binary-data`.`binary-data-signature` AS `signature`,`last_modified_revision`,`last_modified_revision`,`file_revision`"
        << " FROM   `case-binary-data` JOIN `binary-data` ON `binary-data`.signature = `case-binary-data`.`binary-data-signature` AND `case-id` = @caseID "
        << " LEFT JOIN( `binary-sync-history` JOIN `sync_history`ON `sync_history`.`id` = `binary-sync-history`.`sync-history-id` AND `device_id` = @dev) " 
        << " ON `case-binary-data`.`binary-data-signature` = `binary-sync-history`.`binary-data-signature`";
    sql << whereClause.str().c_str() << ") AS T1";
    sql << " WHERE  ( `file_revision` IS NULL OR `file_revision` < last_modified_revision ) GROUP BY `T1`.`signature`";

    SQLiteStatement case_binary_signatures_statement(m_db, sql.str().c_str());

    case_binary_signatures_statement.Bind("@caseID", caseUuid);
    case_binary_signatures_statement.Bind("@dev", deviceId);

    int queryResult;
    while ((queryResult = case_binary_signatures_statement.Step()) == SQLITE_ROW)
    {
        std::string signature = case_binary_signatures_statement.GetColumn<std::string>(0);
        //avoid inserting duplicate binary items from other cases by looking at the excludes and update the exclude list
        if (md5ExcludeKeys.find(signature) == md5ExcludeKeys.cend()) {//if the key is not found in the excludes add it to md5Signatures
            md5Signatures.insert(UTF8Convert::UTF8ToWide(signature)); 
            md5ExcludeKeys.insert(std::move(signature));
        }
    }

    if (queryResult != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);

    return md5Signatures;
}


void SQLiteRepository::GetBinaryCaseItemsModifiedSinceRevision(const Case* data_case,
    std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& caseBinaryItems, std::set<std::string>& md5ExcludeKeys,
    uint64_t& totalBinaryItemsByteSize, DeviceId deviceId /*= CString()*/)
{
    if( data_case == nullptr || !data_case->GetCaseMetadata().UsesBinaryData() )
        return;

    //Get the list of binary data signatures for this case for items that have not been sent to the server
    std::set<std::wstring> signatures_to_sync = GetBinarySignaturesModifiedSinceRevision(data_case->GetUuid(), md5ExcludeKeys, deviceId);

    data_case->ForeachDefinedBinaryCaseItem(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            try
            {
                const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
                const std::wstring& signature = binary_data_accessor.GetBinaryDataMetadata().GetBinaryDataKey();

                // only include data that has not already been processed
                auto signatures_to_sync_lookup = signatures_to_sync.find(signature);

                if( signatures_to_sync_lookup != signatures_to_sync.end() )
                {
                    totalBinaryItemsByteSize += binary_data_accessor.GetBinaryDataSize();
                    caseBinaryItems.emplace_back(&binary_case_item, index);

                    //  remove this item to avoid processing duplicates
                    signatures_to_sync.erase(signatures_to_sync_lookup);
                }
            }
            catch(...) { ASSERT(false); }
        });

    ASSERT(signatures_to_sync.empty());
}


SyncHistoryEntry SQLiteRepository::GetLastSyncForDevice(DeviceId deviceId, SyncDirection direction) const
{
    SQLiteStatement statement(m_db, m_stmtRevisionByDevice,
        "SELECT id, file_revision, device_id, device_name, direction, universe, timestamp, server_revision, partial, last_id FROM sync_history WHERE device_id=? AND direction=? ORDER BY id DESC LIMIT 1");
    statement.Bind(1, deviceId).Bind(2, (int) direction);
    int queryResult = statement.Step();
    if (queryResult == SQLITE_DONE) {
        return SyncHistoryEntry();
    } else if (queryResult == SQLITE_ROW) {
        return SyncHistoryEntry(statement.GetColumn<int>(0),
            statement.GetColumn<int>(1),
            statement.GetColumn<CString>(2),
            statement.GetColumn<CString>(3),
            (SyncDirection) statement.GetColumn<int>(4),
            statement.GetColumn<CString>(5),
            (time_t) statement.GetColumn<int>(6),
            statement.GetColumn<CString>(7),
            (SyncHistoryEntry::SyncState) statement.GetColumn<int>(8),
            statement.GetColumn<CString>(9));
    } else {
        throw SQLiteErrorWithMessage(m_db);
    }
}

std::vector<SyncHistoryEntry> SQLiteRepository::GetSyncHistory(DeviceId deviceId /* = CString()*/, SyncDirection direction/* = SyncDirection::Both*/, int startSerialNumber/* = 0*/)
{
    SQLiteStatement statement(m_db, m_stmtRevisionsByDeviceSince,
        "SELECT id, file_revision, device_id, device_name, direction, universe, timestamp, server_revision, partial, last_id FROM sync_history "
        "WHERE id >= @id AND (@dev='' OR device_id=@dev) AND (@dir = 3 OR @dir = direction) ORDER BY id ASC");
    statement.Bind("@id", startSerialNumber).Bind("@dev", deviceId).Bind("@dir", (int) direction);
    std::vector<SyncHistoryEntry> entries;
    int queryResult;
    while ((queryResult = statement.Step()) == SQLITE_ROW) {

        // For legacy files with no device name use device id
        CString historyDeviceId = statement.GetColumn<CString>(2);
        CString historyDeviceName = statement.IsColumnNull(3) ? historyDeviceId : statement.GetColumn<CString>(3);

        entries.emplace_back(
            statement.GetColumn<int>(0),
            statement.GetColumn<int>(1),
            historyDeviceId,
            historyDeviceName,
            (SyncDirection) statement.GetColumn<int>(4),
            statement.GetColumn<CString>(5),
            (time_t) statement.GetColumn<int>(6),
            statement.GetColumn<CString>(7),
            (SyncHistoryEntry::SyncState) statement.GetColumn<int>(8),
            statement.GetColumn<CString>(9));
    }

    if (queryResult != SQLITE_DONE)
        throw SQLiteErrorWithMessage(m_db);

    return entries;
}

bool SQLiteRepository::IsValidFileRevision(int revisionNumber) const
{
    SQLiteStatement statement(m_db, m_stmtRevisionByNumber,
        "SELECT 1 FROM file_revisions WHERE id=?");
    statement.Bind(1, revisionNumber);
    int queryResult = statement.Step();
    if (queryResult == SQLITE_DONE) {
        return false;
    } else if (queryResult == SQLITE_ROW) {
        return true;
    } else {
        throw SQLiteErrorWithMessage(m_db);
    }
}

bool SQLiteRepository::IsPreviousSync(int fileRevision, CString deviceId) const
{
    SQLiteStatement statement(m_db, m_stmtIsPrevSync,
        "SELECT 1 FROM sync_history WHERE file_revision=? AND device_id=?");
    statement.Bind(1, fileRevision);
    statement.Bind(2, deviceId);
    int queryResult = statement.Step();
    if (queryResult == SQLITE_DONE) {
        return false;
    }
    else if (queryResult == SQLITE_ROW) {
        return true;
    }
    else {
        throw SQLiteErrorWithMessage(m_db);
    }
}

int SQLiteRepository::AddSyncHistoryEntry(int fileRevision, DeviceId deviceId, CString deviceName, CString userName, SyncDirection direction,
    CString universe, CString serverRevision, SyncHistoryEntry::SyncState state, CString lastUuid)
{
    // reset any cached sync times
    m_syncTimeCache.reset();

    SQLiteStatement statement(m_db, m_stmtInsertRevision, "INSERT INTO sync_history (file_revision, device_id,device_name,user_name,universe,direction,server_revision,partial,last_id) "
        "VALUES (?,?,?,?,?,?,?,?,?)");
    statement.Bind(1, fileRevision)
        .Bind(2, deviceId)
        .Bind(3, deviceName)
        .Bind(4, userName)
        .Bind(5, universe)
        .Bind(6, (int) direction)
        .Bind(7, serverRevision)
        .Bind(8, (int) state)
        .Bind(9, lastUuid);
    if (statement.Step() == SQLITE_DONE)
        return (int) sqlite3_last_insert_rowid(m_db);
    else
        throw SQLiteErrorWithMessage(m_db);
}

int SQLiteRepository::AddFileRevision()
{
    SQLiteStatement statement(m_db, m_stmtInsertLocalRevision, "INSERT INTO file_revisions (device_id) VALUES (?)");
    statement.Bind(1, m_deviceId);
    if (statement.Step() == SQLITE_DONE)
        return (int) sqlite3_last_insert_rowid(m_db);
    else
        throw SQLiteErrorWithMessage(m_db);
}

void SQLiteRepository::SetSyncRevisionPartial(int syncId, SyncHistoryEntry::SyncState state,
    CString serverRevision, CString lastCaseUuid, int fileRevision)
{
    SQLiteStatement statement(m_db, m_stmtSetSyncRevLastId, "UPDATE sync_history SET partial=?, last_id=?, server_revision=?, file_revision=? WHERE id=?");
    statement.Bind(1, (int) state).Bind(2, lastCaseUuid).Bind(3, serverRevision).Bind(4, fileRevision).Bind(5, syncId);
    if (statement.Step() != SQLITE_DONE) {
        throw SQLiteErrorWithMessage(m_db);
    }
}

void SQLiteRepository::SetSyncRevisionComplete(int syncId)
{
    SQLiteStatement statement(m_db, m_stmtClearSyncRevLastId, "UPDATE sync_history SET partial=0,last_id=NULL WHERE id=?");
    statement.Bind(1, syncId);
    if (statement.Step() != SQLITE_DONE) {
        throw SQLiteErrorWithMessage(m_db);
    }
}


void SQLiteRepository::StartTransaction()
{
    if (m_transaction_start_count == 0) {
        if (sqlite3_exec(m_db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK)
            throw SQLiteErrorWithMessage(m_db);
        m_iInsertInTransactionCounter = 0;
    }
    ++m_transaction_start_count;
}

void SQLiteRepository::EndTransaction()
{
    --m_transaction_start_count;
    if (m_transaction_start_count == 0) {
        m_transaction_file_revision = -1;
        if (sqlite3_exec(m_db, "COMMIT", NULL, NULL, NULL) != SQLITE_OK) {
            throw SQLiteErrorWithMessage(m_db);
        }
    }
}

void SQLiteRepository::UpdateFilePosition(Case& data_case)
{
    SQLiteStatement get_file_pos(m_stmtGetFileOrderFromUuid);
    get_file_pos.Bind(1, data_case.GetUuid());
    if (get_file_pos.Step() != SQLITE_ROW) {
        throw SQLiteErrorWithMessage(m_db);
    }
    data_case.SetPositionInRepository(get_file_pos.GetColumn<double>(0));
}



struct SyncTimeCache
{
    struct SyncDetails
    {
        double timestamp;
        std::wstring universe;
        std::optional<std::wstring> last_uuid_of_partial_sync;
    };

    std::map<std::wstring, std::map<int, std::vector<SyncDetails>>> device_identifier_to_file_revision_to_sync_details_map;
};

std::optional<double> SQLiteRepository::GetSyncTime(const std::wstring& device_identifier, const std::wstring& case_uuid) const
{
    if( m_syncTimeCache == nullptr )
        m_syncTimeCache = std::make_unique<SyncTimeCache>();

    auto step_statement = [&](SQLiteStatement& statement) -> int
    {
        int result = statement.Step();

        if( result != SQLITE_ROW && result != SQLITE_DONE )
            throw SQLiteErrorWithMessage(m_db);

        return result;
    };


    // first get the sync details for the device identifier
    const std::map<int, std::vector<SyncTimeCache::SyncDetails>>* file_revision_to_sync_details_map = nullptr;
    const auto& file_revision_to_sync_details_map_lookup = m_syncTimeCache->device_identifier_to_file_revision_to_sync_details_map.find(device_identifier);

    if( file_revision_to_sync_details_map_lookup != m_syncTimeCache->device_identifier_to_file_revision_to_sync_details_map.cend() )
    {
        file_revision_to_sync_details_map = &file_revision_to_sync_details_map_lookup->second;
    }

    else
    {
        // the device identifier can be either the device name or the device ID; we will
        // lookup the device assuming the identifier is the name so that we can make sure that
        // we include all relevant sync history (for example, if a server is sometimes localhost and
        // other times the IP address, then this would ensure that all sync history is included)
        std::wstring device_id = device_identifier;
        std::wstring device_name;

        if( !device_identifier.empty() )
        {
            // when searcing by device name, do a case-insensitive search based
            // on the beginning of the string so a name like .../api will match with
            // an entry like .../api/
            SQLiteStatement device_id_from_name_statement(m_db,
                "SELECT `device_id` FROM `sync_history` "
                "WHERE INSTR(LOWER(`device_name`), LOWER(@dn)) = 1 "
                "LIMIT 1;");
            device_id_from_name_statement.Bind("@dn", device_identifier);

            if( step_statement(device_id_from_name_statement) == SQLITE_ROW )
            {
                device_id = device_id_from_name_statement.GetColumn<std::wstring>(0);
                device_name = device_identifier;
            }
        }


        // now get all of the sync times for this device
        std::map<int, std::vector<SyncTimeCache::SyncDetails>> new_file_revision_to_sync_details_map;

        std::wstring sync_details_sql = _T("SELECT `file_revision`, `timestamp`, `universe`, `partial`, `last_id` ")
                                        _T("FROM `sync_history` ");
        std::map<const char*, const std::wstring*> strings_to_bind;

        if( !device_id.empty() )
        {
            sync_details_sql.append(_T("WHERE `device_id` = @di "));
            strings_to_bind["@di"] = &device_id;
        }

        if( !device_name.empty() )
        {
            sync_details_sql.append(device_id.empty() ? _T("WHERE ") : _T("OR "));
            sync_details_sql.append(_T("INSTR(LOWER(`device_name`), LOWER(@dn)) = 1 "));
            strings_to_bind["@dn"] = &device_name;
        }

        sync_details_sql.append(_T("ORDER BY `id`;"));

        SQLiteStatement sync_details_statement(m_db, sync_details_sql);

        for( const auto& [parameter_name, value] : strings_to_bind )
            sync_details_statement.Bind(parameter_name, *value);

        while( step_statement(sync_details_statement) == SQLITE_ROW )
        {
            int file_revision = sync_details_statement.GetColumn<int>(0);
            auto& sync_details = new_file_revision_to_sync_details_map[file_revision].emplace_back();

            sync_details.timestamp = sync_details_statement.GetColumn<double>(1);
            sync_details.universe = sync_details_statement.GetColumn<std::wstring>(2);

            int partial = sync_details_statement.GetColumn<int>(3);

            if( partial != static_cast<int>(SyncHistoryEntry::SyncState::Complete) )
            {
                if( sync_details_statement.IsColumnNull(4) )
                {
                    sync_details.last_uuid_of_partial_sync.emplace();
                }

                else
                {
                    sync_details.last_uuid_of_partial_sync = sync_details_statement.GetColumn<std::wstring>(4);
                }
            }
        }

        file_revision_to_sync_details_map = &m_syncTimeCache->device_identifier_to_file_revision_to_sync_details_map.try_emplace(
            device_identifier, new_file_revision_to_sync_details_map).first->second;
    }


    // we are done if there has never been a sync with this device...
    if( file_revision_to_sync_details_map->empty() )
        return std::nullopt;

    // ...or if not querying for the sync time of a specific case
    if( case_uuid.empty() )
        return file_revision_to_sync_details_map->crbegin()->second.back().timestamp;

    // otherwise see what revision the case is currently at
    SQLiteStatement get_case_revision_statement(m_db, m_stmtGetCaseRev,
        "SELECT `key`, `last_modified_revision` FROM `cases` WHERE `id` = @id LIMIT 1;");
    get_case_revision_statement.Bind("@id", case_uuid);

    if( step_statement(get_case_revision_statement) == SQLITE_ROW )
    {
        // see if there was a sync with this device at or after the case's revision number
        std::wstring case_key = get_case_revision_statement.GetColumn<std::wstring>(0);
        int case_revision = get_case_revision_statement.GetColumn<int>(1);

        for( auto sync_details_itr = file_revision_to_sync_details_map->lower_bound(case_revision);
             sync_details_itr != file_revision_to_sync_details_map->cend();
             ++sync_details_itr )
        {
            for( const SyncTimeCache::SyncDetails& sync_details : sync_details_itr->second )
            {
                // skip syncs where the case would not have been synced due to the use of a universe
                if( !SO::StartsWith(case_key, sync_details.universe) )
                    continue;

                // skip partial syncs in which this case was not synced
                if( sync_details.last_uuid_of_partial_sync.has_value() && *sync_details.last_uuid_of_partial_sync < case_key )
                    continue;

                // finally here is the case's sync time
                return sync_details.timestamp;
            }
        }
    }

    return std::nullopt;
}
