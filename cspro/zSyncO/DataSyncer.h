#pragma once

#include <zSyncO/SyncRunner.h>

struct DataSyncStatistics;
class SyncHistoryEntry;


// --------------------------------------------------------------------------
// DataSyncer runs a data synchronization. The constructor's parameters are
// all references, so the class cannot outlive any of these objects.
// --------------------------------------------------------------------------

class DataSyncer
{
public:
    DataSyncer(ISyncService& sync_service, const ConnectResponse& connect_response,
               std::shared_ptr<SyncListener> sync_listener, const DeviceId& device_id,
               ISyncableDataRepository& syncable_data_repository, const std::string& universe);

    DataSyncStatistics Sync(SyncDirection sync_direction);

private:
    DataSyncStatistics SyncGet();
    DataSyncStatistics SyncPut();

    std::optional<SyncHistoryEntry> GetRevisionFromLastSync(SyncDirection sync_direction) const;

    std::vector<std::string> GetPutRevisionsSince(int start_serial_number) const;

private:
    ISyncService& m_syncService;
    const ConnectResponse& m_connectResponse;
    std::shared_ptr<SyncListener> m_syncListener;
    const DeviceId& m_deviceId;
    ISyncableDataRepository& m_syncableDataRepository;
    const std::string& m_universe;
};
