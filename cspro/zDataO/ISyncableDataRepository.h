#pragma once

#include <zDataO/DataRepository.h>
#include <zToolsO/span.h>
#include <zAppO/SyncTypes.h>
#include <zSyncO/DataSyncStatistics.h>

class SyncBinaryDataUploadManager;
class SyncHistoryEntry;


class ISyncableDataRepository : public DataRepository
{
protected:
    using DataRepository::DataRepository;

public:
    ISyncableDataRepository* GetSyncableDataRepository() override { return this; }

    // Start syncing cases from a remote repository with this repository.
    virtual void StartSync(DeviceId server_device_id, std::string remote_device_name, std::string username, SyncDirection direction, std::string universe,
                           bool use_remote_case_on_conflict) = 0;

    // Add cases received from remote server to the repository.
    // Must call StartSync first and EndSync after for proper bookkeeping of revision history.
    // Returns the client revision number.
    virtual int SyncCasesFromRemote(const std::vector<std::shared_ptr<Case>>& cases_received, const std::string& server_revision) = 0;

    // Mark local cases sent to remote server as part of sync.
    // Must call StartSync first and EndSync after for proper bookkeeping of revision history.
    virtual void MarkCasesSentToRemote(cs::span<const Case* const> cases_sent, const SyncBinaryDataUploadManager* sync_binary_data_upload_manager,
                                       const std::string& server_revision, int client_revision) = 0;

    // Finish sync started with StartSync.
    virtual void EndSync() = 0;

    // Get info about the last sync.
    virtual DataSyncStatistics GetLastSyncStats() const = 0;

    // Clear the binary sync history for the repository.
    virtual void ClearBinarySyncHistory(const DeviceId& server_device_id, int client_revision = -1) = 0;

    // Get only cases that were modified since the specified client revision.
    virtual std::unique_ptr<CaseIterator> GetCasesModifiedSinceRevisionIterator(int client_revision, const std::string& last_case_uuid, const std::string& universe,
                                                                                size_t limit = std::numeric_limits<size_t>::max(), size_t* out_case_count = nullptr, int* out_last_client_revision = nullptr,
                                                                                cs::cref_optional<DeviceId> ignore_gets_from_device_id = std::nullopt,
                                                                                cs::cref_optional<std::vector<std::string>> revisions_to_exclude = std::nullopt) = 0;

    // Add the signatures of the case's binary case item's not synced with the remote repository.
    virtual void AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, const DeviceId& server_device_id, std::vector<std::string>& signatures_to_sync) = 0;

    // Get sync history entry last time specified device was synced.
    virtual std::optional<SyncHistoryEntry> GetLastSyncForDevice(const DeviceId& device_id, SyncDirection direction) const = 0;

    // Get all syncs since for a device since a particular serial number.
    // The sync history is returned in descending order (most recent first).
    // If device_id is empty, then syncs for all devices are returned.
    // If direction is not defined, then syncs in both directions are returned.
    virtual std::vector<SyncHistoryEntry> GetSyncHistory(const DeviceId& device_id = DeviceId(), std::optional<SyncDirection> direction = std::nullopt,
                                                         std::optional<int> start_serial_number = std::nullopt, size_t limit = std::numeric_limits<size_t>::max()) = 0;

    // Check if the client revision exists in the repository.
    virtual bool IsValidClientRevision(int client_revision) const = 0;

    // Determine if a previous sync is in the repository.
    virtual bool IsPreviousSync(int client_revision, const DeviceId& device_id) const = 0;

    // Process a request from the synctime logic function.
    virtual std::optional<double> GetSyncTime(const std::string& device_identifier, const std::string& case_uuid) const = 0;
};
