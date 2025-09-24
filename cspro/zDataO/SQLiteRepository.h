#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zDataO/SyncHistoryEntry.h>

class BinaryCaseItem;
class CDictItem;
struct ISQLiteQuestionnaireSerializer;
struct sqlite3;
struct sqlite3_stmt;
class SQLiteStatement;
struct SyncTimeCache;


class ZDATAO_API SQLiteRepository : public ISyncableDataRepository
{
    friend class SQLiteRepositoryCaseIterator;

protected:
    SQLiteRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type, DeviceId deviceId);

public:
    SQLiteRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type, DeviceId deviceId);
    ~SQLiteRepository();

    sqlite3* GetSqlite() { return m_db; } // for the sqlquery function

    void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) override;
    void ToggleReadWriteMode() override;
    void Close() override;
    void DeleteRepository() override;
    bool ContainsCase(const std::string& key) override;
    void PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository) override;
    DataRepositoryUniqueCaseIdentifer GetUniqueCaseIdentifer(const CaseKey& case_key) override;
    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
                                       const CaseIteratorParameters* start_parameters = nullptr) override;
    void ReadCase(Case& data_case, const std::string& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void ReadCaseByUuid(Case& data_case, const std::string& uuid) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/) override;
    void CommitTransactionIfTooBig();
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    size_t GetNumberCases() override;
    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content,
                                                 const CaseIteratorSettings& iterator_settings,
                                                 size_t offset = 0, size_t limit = SIZE_MAX) override;

    void StartTransaction() override;
    void EndTransaction() override;

    void StartSync(DeviceId server_device_id, std::string remote_device_name, std::string username, SyncDirection direction, std::string universe,
                   bool use_remote_case_on_conflict) override;
    int SyncCasesFromRemote(const std::vector<std::shared_ptr<Case>>& cases_received, const std::string& server_revision) override;
    void MarkCasesSentToRemote(cs::span<const Case* const> cases_sent, const SyncBinaryDataUploadManager* sync_binary_data_upload_manager,
                               const std::string& server_revision, int client_revision) override;
    void ClearBinarySyncHistory(const DeviceId& server_device_id, int client_revision = -1) override;
    void EndSync() override;
    SyncStats GetLastSyncStats() const override;
    std::unique_ptr<CaseIterator> GetCasesModifiedSinceRevisionIterator(int client_revision, const std::string& last_case_uuid, const std::string& universe,
                                                                        size_t limit = std::numeric_limits<size_t>::max(), size_t* out_case_count = nullptr, int* out_last_client_revision = nullptr,
                                                                        cs::cref_optional<DeviceId> ignore_gets_from_device_id = std::nullopt,
                                                                        cs::cref_optional<std::vector<std::string>> revisions_to_exclude = std::nullopt) override;
    void AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, const DeviceId& server_device_id, std::vector<std::string>& signatures_to_sync) override;
    std::optional<SyncHistoryEntry> GetLastSyncForDevice(const DeviceId& device_id, SyncDirection direction) const override;
    std::vector<SyncHistoryEntry> GetSyncHistory(const DeviceId& device_id = DeviceId(), SyncDirection direction = SyncDirection::Both, int start_serial_number = 0) override;
    bool IsValidClientRevision(int client_revision) const override;
    bool IsPreviousSync(int client_revision, const DeviceId& device_id) const override;
    std::optional<double> GetSyncTime(const std::string& device_identifier, const std::string& case_uuid) const override;

    static std::unique_ptr<CDataDict> GetEmbeddedDictionary(const ConnectionString& connection_string);

protected:
    virtual const char* GetFileExtension() const;
    static int OpenSQLiteDatabaseFile(const ConnectionString& connection_string, sqlite3** ppDb, int flags);
    virtual int OpenSQLiteDatabase(const ConnectionString& connection_string, sqlite3** ppDb, int flags);
    static std::unique_ptr<CDataDict> ReadDictionaryFromDatabase(sqlite3* db);

private:
    void Open(DataRepositoryOpenFlag open_flag) override;

    bool CreateDatabaseFile();
    void OpenDatabaseFile();
    bool ReadCaseFromUuid(std::unique_ptr<Case>& data_case, const std::string& uuid);
    void ReadCaseFromDatabase(Case& data_case, SQLiteStatement& get_case_statement);
    int AddSyncHistoryEntry(SyncHistoryEntry::SyncState state, const std::string& last_case_uuid);
    int64_t AddFileRevision();
    void SetSyncRevisionPartial(int sync_id, SyncHistoryEntry::SyncState state, const std::string& server_revision, const std::string& last_case_uuid, int64_t client_revision);
    void SetSyncRevisionComplete(int sync_id);
    void SyncCase(Case& data_case, int64_t client_revision, bool bNewCase);
    void InsertVectorClock(const Case& data_case);
    void UpdateVectorClock(const Case& data_case);
    void ClearNotes(const Case& data_case);
    void WriteNotes(const Case& data_case);
    void IncrementVectorClock(const std::string& uuid);
    void IncrementVectorClock(double position_in_repository);
    int UpdateCase(const Case& data_case, int64_t revision);
    int InsertCase(const Case& data_case, int64_t revision);
    int InsertOrUpdateCase(const Case& data_case, int64_t revision, sqlite3_stmt* pStmt);
    void BindPartialSave(const Case& data_case, SQLiteStatement &insertCase);
    void CreatePreparedStatements();
    void ClearPreparedStatements();
    double GetInsertPosition(double insert_before_position_in_repository);
    std::unique_ptr<SQLiteStatement> GetKeySearchIteratorStatement(const CaseIteratorSettings& iterator_settings,
                                                                   size_t offset, size_t limit, const char* base_sql) const;
    void WriteIteratorSelectFromSql(std::stringstream& sql, CaseIterationContent iteration_content) const;
    void UpdateDictionary(sqlite3* pDB);
    void ReconcileDictionaries(sqlite3** pDB);
    int GetSchemaVersion(sqlite3* pDB) const;
    void MigrateFromSchemaVersion1(sqlite3* pDB);
    void AddDeviceNameUserNameColumnsToSyncHistory(sqlite3* pDB);
    bool MissingDeviceNameColumnInSyncHistory(sqlite3* pDB);
    bool MissingBinaryTable(sqlite3* pDB);
    void AddBinaryTable(sqlite3* pDB);
    bool MissingBinarySyncHistoryTable(sqlite3* pDB);
    void AddBinarySyncHistoryTables(sqlite3* pDB);

    void AddBinaryItemsSyncHistory(const std::string& signature, int sync_id);
    void AddBinaryItemsSyncHistory(const std::vector<std::shared_ptr<Case>>& cases_received, int sync_id);

    void MakeDatabaseTemporarilyWriteable(sqlite3** db);
    void EndMakeDatabaseTemporarilyWriteable(sqlite3** db);
    void UpdateFilePosition(Case& data_case);
    static std::string GetDictionaryStructureMd5(sqlite3* db);

private:
    mutable sqlite3* m_db;
    const DeviceId m_deviceId;
    int64_t m_transactionClientRevision;
    int m_transactionStartCount;
    int m_iInsertInTransactionCounter;
    std::unique_ptr<ISQLiteQuestionnaireSerializer> m_questionnaireSerializer;

    struct SyncParams
    {
        int current_sync_id;
        int64_t current_client_revision;
        DeviceId remote_device_id;
        std::string remote_device_name;
        std::string username;
        SyncDirection direction;
        std::string universe;
        bool use_remote_case_on_conflict;
        std::string server_revision;
    };

    SyncParams m_currentSyncParams;
    SyncStats m_currentSyncStats;

    sqlite3_stmt* m_stmtInsertCase;
    sqlite3_stmt* m_stmtUpdateCase;
    sqlite3_stmt* m_stmtSelectCases;
    mutable sqlite3_stmt* m_stmtCountCases;
    sqlite3_stmt* m_stmtGetCaseByKey;
    sqlite3_stmt* m_stmtGetCaseByFileOrder;
    sqlite3_stmt* m_stmtGetCaseById;
    mutable sqlite3_stmt* m_stmtContainsCase;
    sqlite3_stmt* m_stmtModifyDeleteStatus;
    sqlite3_stmt* m_stmtInsertRevision;
    sqlite3_stmt* m_stmtInsertLocalRevision;
    sqlite3_stmt* m_stmtUpdateClock;
    sqlite3_stmt* m_stmtIncrementClock;
    sqlite3_stmt* m_stmtNewClock;
    mutable sqlite3_stmt* m_stmtGetClock;
    sqlite3_stmt* m_stmtSyncCase;
    sqlite3_stmt* m_stmtInsertBinarySyncHistory;
    sqlite3_stmt* m_stmtDeleteBinarySyncHistory;
    sqlite3_stmt*  m_stmtArchiveBinarySyncHistory;
    mutable sqlite3_stmt* m_stmtRevisionByNumber;
    mutable sqlite3_stmt* m_stmtIsPrevSync;
    mutable sqlite3_stmt* m_stmtRevisionByDevice;
    mutable sqlite3_stmt* m_stmtRevisionsByDeviceSince;
    mutable sqlite3_stmt* m_stmtCaseIdentifiersFromKey;
    mutable sqlite3_stmt* m_stmtCaseIdentifiersFromUuid;
    mutable sqlite3_stmt* m_stmtCaseIdentifiersFromFileOrder;
    sqlite3_stmt* m_stmtGetNotes;
    sqlite3_stmt* m_stmtClearNotes;
    sqlite3_stmt* m_stmtUpdateNote;
    sqlite3_stmt* m_stmtCaseExists;
    sqlite3_stmt* m_stmtGetPrevFileOrder;
    sqlite3_stmt* m_stmtSetSyncRevLastId;
    sqlite3_stmt* m_stmtClearSyncRevLastId;
    sqlite3_stmt* m_stmtGetFileOrderFromUuid;

    // for synctime
    mutable std::unique_ptr<SyncTimeCache> m_syncTimeCache;
    mutable sqlite3_stmt* m_stmtGetCaseRev;
};
