#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/ISyncableDataRepository.h>

class BinaryCaseItem;
class CDictItem;
struct ISQLiteQuestionnaireSerializer;
struct sqlite3;
struct sqlite3_stmt;
class SQLiteStatement;
struct SyncTimeCache;


class ZDATAO_API SQLiteRepository : public ISyncableDataRepository {
    friend class SQLiteRepositoryCaseIterator;

protected:
    SQLiteRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type, DeviceId deviceId);

public:
    SQLiteRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type, DeviceId deviceId)
        :   SQLiteRepository(DataRepositoryType::SQLite, std::move(case_access), access_type, deviceId)
    {
    }

    ~SQLiteRepository();

    sqlite3* GetSqlite() { return m_db; } // for the sqlquery function

    void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) override;

    void Close() override;
    void DeleteRepository() override;
    bool ContainsCase(const CString& key) const override;
    void PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const override;
    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
	    const CaseIteratorParameters* start_parameters = nullptr) const override;
    void ReadCase(Case& data_case, const CString& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/) override;
    void CommitTransactionIfTooBig();
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    size_t GetNumberCases() const override;
    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) const override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
        std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order, 
        const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;
    void StartTransaction() override;
    void EndTransaction() override;
    void StartSync(DeviceId remoteDeviceId, CString remoteDeviceName, CString userName, SyncDirection direction,
        CString universe, bool bUpdateOnConflict) override;
    int SyncCasesFromRemote(const std::vector<std::shared_ptr<Case>>& cases_received, CString serverRevision) override;
    void MarkCasesSentToRemote(const std::vector<std::shared_ptr<Case>>& cases_sent, std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryDataItems, CString serverRevision, int clientRevision) override;
    void ClearBinarySyncHistory(CString serverDeviceId, int fileRevision = -1) override;
    void EndSync() override;
    SyncStats GetLastSyncStats() const override;
    std::unique_ptr<CaseIterator> GetCasesModifiedSinceRevisionIterator(int fileRevision, CString lastUuid,
        CString universe, int limit = INT_MAX,
        int* pCaseCount = 0, int *pLastFileRev = 0,
        CString ignoreGetsFromDeviceId = CString(),
        const std::vector<std::wstring>& ignoreRevisions = std::vector<std::wstring>()) override;
    void GetBinaryCaseItemsModifiedSinceRevision(const Case* data_case, std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& caseBinaryItems,
        std::set<std::string>& md5ExcludeKeys, uint64_t& totalBinaryItemsByteSize, DeviceId deviceId = CString()) override;

    SyncHistoryEntry GetLastSyncForDevice(DeviceId deviceId, SyncDirection direction) const override;
    std::vector<SyncHistoryEntry> GetSyncHistory(DeviceId deviceId, SyncDirection direction = SyncDirection::Both, int startSerialNumber = 0) override;
    bool IsValidFileRevision(int revisionNumber) const override;
    bool IsPreviousSync(int fileRevision, CString deviceId) const override;
    std::optional<double> GetSyncTime(const std::wstring& device_identifier, const std::wstring& case_uuid) const override;

    static std::unique_ptr<CDataDict> GetEmbeddedDictionary(const ConnectionString& connection_string);

protected:
    virtual const TCHAR* GetFileExtension() const;
    static int OpenSQLiteDatabaseFile(const ConnectionString& connection_string, sqlite3** ppDb, int flags);
    virtual int OpenSQLiteDatabase(const ConnectionString& connection_string, sqlite3** ppDb, int flags);
    static std::unique_ptr<CDataDict> ReadDictFromDatabase(sqlite3* pDB);

private:
    void Open(DataRepositoryOpenFlag open_flag) override;

    bool CreateDatabaseFile();
    void OpenDatabaseFile();
    bool ReadCaseFromUuid(Case& data_case, CString uuid);
    bool ReadCaseFromDatabase(Case& data_case, SQLiteStatement& get_case_statement);
    int AddSyncHistoryEntry(int fileRevision, DeviceId deviceId, CString deviceName, CString userName, SyncDirection direction, CString universe, CString serverRevision, SyncHistoryEntry::SyncState state, CString lastUuid);
    int AddFileRevision();
    void SetSyncRevisionPartial(int iRevisionNumber, SyncHistoryEntry::SyncState state, CString serverRevision, CString lastCaseUuid, int fileRevision);
    void SetSyncRevisionComplete(int iRevisionNumber);
    void SyncCase(Case& data_case, int fileRevision, bool bNewCase, int currentSyncId);
    void InsertVectorClock(const Case& data_case);
    void UpdateVectorClock(const Case& data_case);
    void ClearNotes(const Case& data_case);
    void WriteNotes(const Case& data_case);
    void IncrementVectorClock(CString uuid);
    void IncrementVectorClock(double position_in_repository);
    int UpdateCase(const Case& data_case, int64_t revision);
    int InsertCase(const Case& data_case, int64_t revision);
    int InsertOrUpdateCase(const Case& data_case, int64_t revision, sqlite3_stmt* pStmt);
    void BindPartialSave(const Case& data_case, SQLiteStatement &insertCase);
    void CreatePreparedStatements();
    void ClearPreparedStatements();
    double GetInsertPosition(double insert_before_position_in_repository);
    std::unique_ptr<SQLiteStatement> GetKeySearchIteratorStatement(size_t offset, size_t limit,
        CaseIterationCaseStatus case_status, std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
        const CaseIteratorParameters* start_parameters, const TCHAR* base_sql) const;
    void WriteIteratorSelectFromSql(std::wstringstream& sql, CaseIterationContent iteration_content) const;
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

    std::set<std::wstring> GetBinarySignaturesModifiedSinceRevision(const CString& caseUuid, std::set<std::string>& md5ExcludeKeys, DeviceId deviceId);

    void AddBinaryItemsSyncHistory(const Case& data_case, int syncHistoryId);
    void AddBinaryItemsSyncHistory(const std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryCaseItems, int syncHistoryId);
    void AddBinaryItemsSyncHistory(const std::set<std::wstring>& synced_signatures, int syncHistoryId);

    void MakeDatabaseTemporarilyWriteable(sqlite3** db);
    void EndMakeDatabaseTemporarilyWriteable(sqlite3** db);
    void UpdateFilePosition(Case& data_case);
    static std::wstring GetDictionaryStructureMd5(sqlite3* db);

private:
    mutable sqlite3* m_db;
    CString m_deviceId;
    int m_transaction_file_revision;
    int m_transaction_start_count;
    int m_iInsertInTransactionCounter;
    ISQLiteQuestionnaireSerializer* m_questionnaireSerializer;

    struct SyncParams
    {
        DeviceId m_remoteDeviceId;
        CString m_remoteDeviceName;
        CString m_userName;
        SyncDirection m_direction;
        CString m_universe;
        int m_currentSyncId;
        int m_currentFileRevision;
        bool m_currentSyncUpdateOnConflict;
        CString m_serverRevision;
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
