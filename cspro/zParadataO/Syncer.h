#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/Log.h>
#include <zToolsO/PointerClasses.h>

namespace Paradata { class Syncer; }
struct sqlite3;
class TemporaryFile;


class ZPARADATAO_API Paradata::Syncer
{
public:
    Syncer(cs::non_null_shared_or_raw_ptr<Log> log);
    Syncer(const std::string& file_path);
    ~Syncer();

    std::string GetLogFilePath() const { return m_log->GetFilePath(); }

    const std::string& GetLogUuid() const { return m_logUuid; }

    const std::string& GetPeerLogUuid() const      { return m_peerLogUuid; }
    void SetPeerLogUuid(std::string peer_log_uuid) { m_peerLogUuid = std::move(peer_log_uuid); }

    std::optional<std::string> GetExtractedSyncableDatabaseFilePath();

    const std::string& GetFilePathForReceivedSyncableDatabase();
    void SetReceivedSyncableDatabases(std::vector<TemporaryFile> received_database_temporary_files);

    void MergeReceivedSyncableDatabases();

    void RunPostSuccessfulSyncTasks();

private:
    struct EventBoundary;

    void FlushAndResetPreparedStatements();

    void SetupSyncTableAndGetLogUuid();

    std::optional<int64_t> GetMaxEventId();

    // extraction methods
    static void ConsolidateEventBoundaries(std::vector<EventBoundary>& event_boundaries);
    void RemovePreviouslySyncedFromSyncableEventBoundaries();

    void CalculateSyncableEventBoundaries();

    void ExtractSyncableDatabase();

    void SetJournalModeAndSynchronous(bool speedup);
    void StartExtraction(const std::string& extracted_events_file_path);
    void StopExtraction();

    void ExtractEvents(Table& event_table, const std::string& where_sql);
    void ExtractLinkingTableData(const Table& table, int64_t start_id);

    bool UpdateLinkingTableStartIds(Table& table, std::optional<int64_t> table_start_id);

    void UpdateEventBoundaries(const std::vector<EventBoundary>& event_boundaries,
                               std::optional<int64_t> delete_events_boundaries_up_to_including_id);

private:
    struct EventBoundary
    {
        int64_t start_event_id;
        int64_t end_event_id;
    };

private:
    // this paradata log
    cs::non_null_shared_or_raw_ptr<Log> m_log;
    sqlite3* m_inputDb;
    std::string m_logUuid;

    // information about the peer
    std::string m_peerLogUuid;
    std::vector<TemporaryFile> m_receivedDatabaseTemporaryFiles;

    // extraction variables
    std::vector<EventBoundary> m_syncableEventBoundaries;
    std::vector<EventBoundary> m_previouslySyncedEventBoundaries;

    std::unique_ptr<TemporaryFile> m_extractedDatabaseTemporaryFile;

    std::string m_journalModeSetting;
    std::string m_synchronousSetting;

    std::map<std::shared_ptr<Table>, std::optional<int64_t>> m_linkingTableStartIds;
};
