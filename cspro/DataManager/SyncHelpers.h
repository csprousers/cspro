#pragma once

#include <zSyncO/SyncRunner.h>

class CSortListCtrl;
class SyncDictionaryInfo;
class SyncTask;


class SyncHelpers
{
public:
    // Sets up the list control for showing available data.
    static void SetUpDataListCtrl(CSortListCtrl& data_list_ctrl);

    // Fills the list control with the queried dictionaries.
    static void FillDataList(CSortListCtrl& data_list_ctrl, std::vector<SyncDictionaryInfo>& queried_dictionaries);

    // Throws an exception if the data source type cannot be used for syncing.
    static void ValidateDataRepositoryTypeValidForSync(const ConnectionString& connection_string);

    // Creates a SyncRunner and optionally connects to a sync service.
    static SyncRunner CreateSyncRunner();
    static SyncRunner::Connection CreateSyncRunnerAndConnect(std::optional<SyncRunner>& sync_runner, const SyncConnectionString& sync_connection_string);

    // Returns SyncRunner::Connection's ISyncService.
    static ISyncService& GetSyncService(SyncRunner::Connection& sync_runner_connection);

    // Creates a sync task for DownloadDataSourceDlg and for production syncs. The returned task can be null.
    static std::unique_ptr<SyncTask> CreateSyncTaskForUnopenedDataRepositories(SyncRunner::Connection sync_runner_connection,
                                                                               const std::string& dictionary_name,
                                                                               const ConnectionString& connection_string,
                                                                               SyncDirection sync_direction,
                                                                               bool production_sync_mode);

private:
    // Reads a dictionary from a sync service.
    static std::unique_ptr<const CDataDict> GetDictionary(SyncRunner& sync_runner, SyncRunner::Connection& sync_runner_connection,
                                                          const std::string& dictionary_name);

};
