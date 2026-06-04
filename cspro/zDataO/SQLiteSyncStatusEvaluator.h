#pragma once

#include <zDataO/SQLiteRepository.h>
#include <zSql/Statement.h>


class SQLiteRepository::SyncStatusEvaluator
{
public:
    SyncStatusEvaluator(SQLiteRepository& repository);
    ~SyncStatusEvaluator();

    void ClearPreparedStatements();

    std::optional<double> GetSyncTime(const std::string& device_identifier, const std::string& case_uuid);

private:
    SQLiteRepository& m_repository;
    Sqlite::Statement m_stmtGetCaseRev;

    struct SyncDetails
    {
        double timestamp;
        std::string universe;
        std::optional<std::string> last_uuid_of_partial_sync;
    };

    std::map<std::string, std::map<int, std::vector<SyncDetails>>> m_deviceIdentifierToFileRevisionToSyncDetailsMap;
};
