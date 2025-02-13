#pragma once

#include <zParadataO/zParadataO.h>
#include <zParadataO/Log.h>

struct sqlite3;
class TemporaryFile;


namespace Paradata
{
    class ZPARADATAO_API Syncer
    {
    private:
        struct EventBoundary
        {
            int64_t start_event_id;
            int64_t end_event_id;
        };

    public:
        Syncer(Log& log);
        ~Syncer();

        const CString& GetLogUuid() const                 { return m_logUuid; }

        const CString& GetPeerLogUuid() const             { return m_peerLogUuid; }
        void SetPeerLogUuid(const CString& peer_log_uuid) { m_peerLogUuid = peer_log_uuid; }

        std::optional<std::wstring> GetExtractedSyncableDatabase();

        const std::wstring& GetFilenameForReceivedSyncableDatabase();
        void SetReceivedSyncableDatabases(std::vector<std::shared_ptr<TemporaryFile>> received_database_temporary_files);

        void MergeReceivedSyncableDatabases();

        void RunPostSuccessfulSyncTasks();

    private:
        void FlushAndResetPreparedStatements();

        void SetupSyncTableAndGetLogUuid();

        std::optional<int64_t> GetMaxEventId();

        // extraction methods
        static void ConsolidateEventBoundaries(std::vector<EventBoundary>& event_boundaries);
        void RemovePreviouslySyncedFromSyncableEventBoundaries();

        void CalculateSyncableEventBoundaries();

        void ExtractSyncableDatabase();

        void SetJournalModeAndSynchronous(bool speedup);
        void StartExtraction(const std::wstring& extracted_events_filename);
        void StopExtraction();

        void ExtractEvents(Table& event_table, const CString& where_sql);
        void ExtractLinkingTableData(std::shared_ptr<Table> table, int64_t start_id);

        bool UpdateLinkingTableStartIds(Table& table, std::optional<int64_t> table_start_id);

        void UpdateEventBoundaries(const std::vector<EventBoundary>& event_boundaries,
                                   std::optional<int64_t> delete_events_boundaries_up_to_including_id);

    private:
        // this paradata log
        Log& m_log;
        sqlite3* m_inputDb;
        CString m_logUuid;

        // information about the peer
        CString m_peerLogUuid;
        std::vector<std::shared_ptr<TemporaryFile>> m_receivedDatabaseTemporaryFiles;

        // extraction variables
        std::vector<EventBoundary> m_syncableEventBoundaries;
        std::vector<EventBoundary> m_previouslySyncedEventBoundaries;

        std::unique_ptr<TemporaryFile> m_extractedDatabaseTemporaryFile;

        CString m_journalModeSetting;
        CString m_synchronousSetting;

        std::map<std::shared_ptr<Table>, std::optional<int64_t>> m_linkingTableStartIds;
    };
}
