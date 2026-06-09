#pragma once

#include <zDataO/SQLiteRepository.h>
#include <zSql/Statement.h>


class SQLiteRepository::SyncStatusEvaluator
{
public:
    SyncStatusEvaluator(SQLiteRepository& repository);

    void ClearPreparedStatements();

    std::optional<double> GetSyncTime(const SharableString& device_identifier, const SharableString& case_uuid);

    void WriteSyncStatus(JsonWriter& json_writer, const JsonNode& json_node, const SharableString& device_id, const SharableString& device_name);

private:
    struct SyncTimeData;

    // Returns the device ID associated with the device name, or an empty string if no such device exists.
    // For ...ForSyncTime, device names are matched in a case-insensitive manner based on the beginning of the string,
    // so a name like .../api would match with an entry like .../api/.
    // For ...ForSyncStatus, the device name must match identically.
    // If ensure_that_only_one_device_matches is true, an exception is thrown if more than one device matches.
    // If ensure_that_a_device_matches is true, an exception is thrown if no device matches.
    // Otherwise, if the device name has no matches, a fake device ID string is returned.
    std::string GetDeviceIdFromNameForSyncTime(const std::string& device_name);
    SharableString GetDeviceIdFromNameForSyncStatus(const std::string& device_name, bool ensure_that_only_one_device_matches, bool ensure_that_a_device_matches);

    // Returns the device ID if only one exists in the sync_history table, throwing an exception if
    // there have been synchronizations with multiple devices. Based on create_fake_device_id_if_no_syncs_have_occurred,
    // a fake device ID string may be returned if there have been no synchronizations.
    std::string GetDeviceIdIfUnique(bool create_fake_device_id_if_no_syncs_have_occurred);

    const std::map<int, std::vector<SyncTimeData>>& GetSyncTimesForDeviceIdentifier(const SharableString& device_identifier);

    // Returns the key / position / UUID, and the revision, matched against a case identifier,
    // returning a blank string for the key if no such case exists.
    template<typename T>
    std::tuple<CaseKey, std::string, int> GetCaseRevisionFromIdentifier(const char* binding_parameter_name, const T& binding_value);

    // Constructs a SyncHistoryData object from a row in a query's result.
    struct SyncHistoryData;
    static std::unique_ptr<SyncHistoryData> CreateSyncHistoryData(Sqlite::Statement& stmt);

    // Writes a SyncHistoryData entry to JSON.
    static void WriteSyncHistoryData(JsonWriter& json_writer, const SyncHistoryData& sync_history_data);
    static void WriteSyncHistoryData(JsonWriter& json_writer, const char* key, const SyncHistoryData& sync_history_data);

    // Used by the "casesPendingSync" and "summary" queries to process the results of the call to
    // SQLiteRepository::GetCasesModifiedSinceRevisionIterator.
    std::unique_ptr<SQLiteRepositoryCaseIterator> CreateCasesModifiedSinceRevisionIterator(
        const SharableString& device_id, const std::string& universe, size_t limit, size_t* out_case_count);

    void WriteSyncStatus_syncServices(JsonWriter& json_writer);

    void WriteSyncStatus_syncHistory(JsonWriter& json_writer, const SharableString& device_id,
                                     const SharableString& device_name, std::optional<uint32_t> limit);

    void WriteSyncStatus_casesPendingSync(JsonWriter& json_writer, SharableString device_id, const SharableString& device_name,
                                          const std::string& universe, std::optional<uint32_t> limit);

    void WriteSyncStatus_caseStatus(JsonWriter& json_writer, const JsonNode& json_node,
                                    const SharableString& device_id, const SharableString& device_name);

    void WriteSyncStatus_summary(JsonWriter& json_writer, SharableString device_id, const SharableString& device_name);

private:
    SQLiteRepository& m_repository;

    Sqlite::Statement m_stmtGetDeviceIdFromNameForSyncTime;
    Sqlite::Statement m_stmtGetDeviceIdFromNameForSyncStatus;
    Sqlite::Statement m_stmtGetUniqueDeviceId;
    Sqlite::Statement m_stmtGetSyncTimeData;
    Sqlite::Statement m_stmtGetCaseRevision;
    Sqlite::Statement m_stmtGetSyncServices;
    Sqlite::Statement m_stmtGetSyncHistory;
    Sqlite::Statement m_stmtGetCaseLastSync;
    Sqlite::Statement m_stmtGetLatestSyncHistory;

    // for synctime
    struct SyncTimeData
    {
        double timestamp;
        std::string universe;
        std::optional<std::string> last_uuid_of_partial_sync;
    };

    std::map<std::string, std::map<int, std::vector<SyncTimeData>>> m_deviceIdentifierToFileRevisionToSyncTimeDataMap;
};
