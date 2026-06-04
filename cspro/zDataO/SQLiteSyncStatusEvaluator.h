#pragma once

#include <zDataO/SQLiteRepository.h>
#include <zSql/Statement.h>


class SQLiteRepository::SyncStatusEvaluator
{
public:
    SyncStatusEvaluator(SQLiteRepository& repository);

    void ClearPreparedStatements();

    std::optional<double> GetSyncTime(const SharableString& device_identifier, const SharableString& case_uuid);

private:
    struct SyncTimeData;

    // Returns the device ID associated with the device name, or an empty string if no such device exists.
    // Device names are matched in a case-insensitive manner based on the beginning of the string,
    // so a name like .../api would match with an entry like .../api/.
    std::string GetDeviceIdFromName(const std::string& device_name);

    const std::map<int, std::vector<SyncTimeData>>& GetSyncTimesForDeviceIdentifier(const SharableString& device_identifier);

    // Returns the case key and revision from a UUID, returning a blank string for the key if no such case exists.
    std::tuple<std::string, int> GetCaseRevisionFromUuid(const std::string& case_uuid);

private:
    SQLiteRepository& m_repository;

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
};
