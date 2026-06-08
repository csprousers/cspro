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
    // Device names are matched in a case-insensitive manner based on the beginning of the string,
    // so a name like .../api would match with an entry like .../api/.
    std::string GetDeviceIdFromName(const std::string& device_name);

    const std::map<int, std::vector<SyncTimeData>>& GetSyncTimesForDeviceIdentifier(const SharableString& device_identifier);

    // Returns the case key and revision from a UUID, returning a blank string for the key if no such case exists.
    std::tuple<std::string, int> GetCaseRevisionFromUuid(const std::string& case_uuid);

    // Constructs a SyncHistoryData object from a row in a query's result.
    struct SyncHistoryData;
    static std::unique_ptr<SyncHistoryData> CreateSyncHistoryData(Sqlite::Statement& stmt);

    // Writes a SyncHistoryData entry to JSON.
    static void WriteSyncHistoryData(JsonWriter& json_writer, const SyncHistoryData& sync_history_data);
    static void WriteSyncHistoryData(JsonWriter& json_writer, const char* key, const SyncHistoryData& sync_history_data);

    void WriteSyncStatus_syncServices(JsonWriter& json_writer);
    void WriteSyncStatus_syncHistory(JsonWriter& json_writer, const SharableString& device_id, const SharableString& device_name);

private:
    SQLiteRepository& m_repository;

    // for synctime
    Sqlite::Statement m_stmtGetDeviceIdFromName;
    Sqlite::Statement m_stmtGetSyncTimeData;
    Sqlite::Statement m_stmtGetCaseRevision;

    struct SyncTimeData
    {
        double timestamp;
        std::string universe;
        std::optional<std::string> last_uuid_of_partial_sync;
    };

    std::map<std::string, std::map<int, std::vector<SyncTimeData>>> m_deviceIdentifierToFileRevisionToSyncTimeDataMap;

    // Data.getSyncStatus
    Sqlite::Statement m_stmtGetSyncServices;
    Sqlite::Statement m_stmtGetSyncHistory;

};
