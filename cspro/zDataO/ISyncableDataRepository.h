#pragma once

#include <zDataO/DataRepository.h>
#include <zDataO/SyncHistoryEntry.h>

class BinaryCaseItem;


class ISyncableDataRepository : public DataRepository
{
protected:
    ISyncableDataRepository(DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type)
        :   DataRepository(type, std::move(case_access), access_type)
    {
    }

public:
    ISyncableDataRepository* GetSyncableDataRepository() override             { return this; }
    const ISyncableDataRepository* GetSyncableDataRepository() const override { return this; }

    /// <summary>
    /// Info on cases synced.
    /// </summary>
    struct SyncStats
    {
        int numNewCasesNotInRepo;
        int numCasesNewerInRepo;
        int numCasesNewerOnRemote;
        int numConflicts;
        int numReceived;
        int numSent;
    };

    /// <summary>
    /// Start syncing Casetainers from a remote repo with this repo.
    /// </summary>
    virtual void StartSync(DeviceId remoteDeviceId, CString remoteDeviceName, CString userName, SyncDirection direction,
        CString universe, bool bUpdateOnConflict) = 0;

    /// <summary>
    /// Add cases received from remote server to the repo.
    /// Must call StartSync first and EndSync after for proper
    /// bookeeping of revision history.
    /// <returns>File revision number</returns>
    /// </summary>
    virtual int SyncCasesFromRemote(const std::vector<std::shared_ptr<Case>>& cases_received, CString serverRevision) = 0;

    /// <summary>
    /// Mark local cases sent to remote server as part of sync.
    /// Must call StartSync first and EndSync after for proper
    /// bookeeping of revision history.
    /// </summary>
    virtual void MarkCasesSentToRemote(const std::vector<std::shared_ptr<Case>>& cases_sent, std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& binaryDataItems, CString serverRevision, int clientRevision) = 0;

    /// <summary>
    /// Finish sync started with StartSync.
    /// </summary>
    virtual void EndSync() = 0;

    /// <summary>
    /// Get info about last sync
    /// </summary>
    virtual SyncStats GetLastSyncStats() const = 0;

    /// <summary>
    /// Get info about last sync
    /// </summary>
    virtual void ClearBinarySyncHistory(CString serverDeviceId, int fileRevision = -1) = 0;

    /// <summary>
    /// Get only cases that were modified since specified file revision
    /// </summary>
    virtual std::unique_ptr<CaseIterator> GetCasesModifiedSinceRevisionIterator(int fileRevision, CString lastUuid, CString universe,
        int limit = INT_MAX, int* pCaseCount = 0,
        int *pLastFileRev = 0, CString ignoreGetsFromDeviceId = CString(),
        const std::vector<std::wstring>& ignoreRevisions = std::vector<std::wstring>()) = 0;

    /// <summary>
    /// Get Binary Data Items of case that have not been sent in earlier revisions
    /// </summary>
    virtual void GetBinaryCaseItemsModifiedSinceRevision(const Case* data_case,
        std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>>& caseBinaryItems, std::set<std::string>& md5ExcludeKeys,
        uint64_t& totalBinaryItemsByteSize, DeviceId deviceId = CString()) = 0;

    /// <summary>
    /// Get sync history entry last time specified device was synced.
    /// </summary>
    virtual SyncHistoryEntry GetLastSyncForDevice(DeviceId deviceId, SyncDirection direction) const = 0;

    /// <summary>
    /// Get all syncs since for a device since a particular serial number.
    /// </summary>
    virtual std::vector<SyncHistoryEntry> GetSyncHistory(DeviceId deviceId = CString(), SyncDirection direction = SyncDirection::Both, int startSerialNumber = 0) = 0;

    /// <summary>
    /// Check if the file revision exists in the repository.
    /// </summary>
    virtual bool IsValidFileRevision(int revisionNumber) const = 0;

    /// <summary>
    /// Determine if a previous sync is in the repository.
    /// </summary>
    virtual bool IsPreviousSync(int fileRevision, CString deviceId) const = 0;

    /// <summary>
    /// Process a request from the synctime logic function.
    /// </summary>
    virtual std::optional<double> GetSyncTime(const std::wstring& device_identifier, const std::wstring& case_uuid) const = 0;
};
