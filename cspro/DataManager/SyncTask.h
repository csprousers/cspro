#pragma once

#include <DataManager/Task.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncRunner.h>


// --------------------------------------------------------------------------
// SyncTask
// --------------------------------------------------------------------------

class SyncTask : public Task
{
public:
    SyncTask(SyncRunner::Connection sync_runner_connection, std::shared_ptr<DataRepository> data_repository,
             SyncDirection sync_direction, std::string universe);

    // Task overrides
    void Run() override final;

protected:
    SyncClient m_syncClient;
    std::shared_ptr<DataRepository> m_dataRepository;
    SyncDirection m_syncDirection;
    std::string m_universe;
};



// --------------------------------------------------------------------------
// SyncTaskForUnopenedDataRepositories
// --------------------------------------------------------------------------

class SyncTaskForUnopenedDataRepositories : public SyncTask
{
public:
    SyncTaskForUnopenedDataRepositories(SyncRunner::Connection sync_runner_connection, std::shared_ptr<DataRepository> data_repository,
                                        SyncDirection sync_direction, bool delete_data_repository_on_error, std::unique_ptr<const CDataDict> dictionary);

    // Task overrides
    void Finalize(Result result) override;

private:
    bool m_deleteDataRepositoryOnError;
    std::unique_ptr<const CDataDict> m_dictionary;
};
