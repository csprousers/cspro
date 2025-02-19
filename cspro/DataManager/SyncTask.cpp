#include "StdAfx.h"
#include "SyncTask.h"
#include "SyncHelpers.h"
#include "TaskRunnerSyncListener.h"
#include <zSyncO/SyncClient.h>


// --------------------------------------------------------------------------
// SyncTask
// --------------------------------------------------------------------------

SyncTask::SyncTask(SyncRunner::Connection sync_runner_connection, std::shared_ptr<DataRepository> data_repository,
                   const SyncDirection sync_direction, std::string universe)
    :   m_syncClient(std::move(sync_runner_connection)),
        m_dataRepository(std::move(data_repository)),
        m_syncDirection(sync_direction),
        m_universe(std::move(universe))
{
    ASSERT(m_dataRepository != nullptr);
}


void SyncTask::Run()
{
    ISyncableDataRepository* const syncable_data_repository = m_dataRepository->GetSyncableDataRepository();

    if( syncable_data_repository == nullptr )
    {
        SyncHelpers::ValidateDataRepositoryTypeValidForSync(m_dataRepository->GetConnectionString());
        throw ProgrammingErrorException();
    }

    m_syncClient.SetSyncListener(std::make_unique<TaskRunnerSyncListener>(m_taskRunner, m_cancelFlag));

    const SyncClient::SyncResult result = m_syncClient.SyncData(m_syncDirection, *syncable_data_repository, m_universe);

    if( result != SyncClient::SyncResult::SYNC_OK )
        throw CSProException("There was an error syncing data.");
}



// --------------------------------------------------------------------------
// SyncTaskForUnopenedDataRepositories
// --------------------------------------------------------------------------

SyncTaskForUnopenedDataRepositories::SyncTaskForUnopenedDataRepositories(SyncRunner::Connection sync_runner_connection, std::shared_ptr<DataRepository> data_repository,
                                                                         const SyncDirection sync_direction, const bool delete_data_repository_on_error,
                                                                         std::unique_ptr<const CDataDict> dictionary)
    :   SyncTask(sync_runner_connection, std::move(data_repository), sync_direction, std::string()),
        m_deleteDataRepositoryOnError(delete_data_repository_on_error),
        m_dictionary(std::move(dictionary))
{
    ASSERT(m_dictionary != nullptr);
}


void SyncTaskForUnopenedDataRepositories::Finalize(const Result result)
{
    try
    {
        // close the data source on success or if unable to delete it
        if( result == Complete || !m_deleteDataRepositoryOnError )
        {
            m_dataRepository->Close();
        }

        // delete the data source on error or cancelation
        else
        {
            m_dataRepository->DeleteRepository();
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
