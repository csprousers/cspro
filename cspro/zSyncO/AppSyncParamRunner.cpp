#include "stdafx.h"
#include "AppSyncParamRunner.h"
#include "SyncClient.h"
#include <zDictO/DDClass.h>
#include <zDataO/ISyncableDataRepository.h>


int AppSyncParamRunner::Run(SyncClient& sync_client, const AppSyncParameters& sync_params, const std::vector<DataRepository*>& data_repositories_to_sync)
{
    std::optional<int> dictionaries_synced;

    for( DataRepository* const data_repository : data_repositories_to_sync )
    {
        ASSERT(data_repository != nullptr);

        ISyncableDataRepository* const syncable_data_repository = data_repository->GetSyncableDataRepository();

        if( syncable_data_repository == nullptr )
        {
            if( sync_client.GetSyncListener() != nullptr )
                sync_client.GetSyncListener()->ReportError(100116, "syncdata", data_repository->GetCaseAccess().GetDataDict().GetName().c_str());

            continue;
        }

        if( !dictionaries_synced.has_value() )
        {
            if( sync_client.Connect(sync_params.sync_connection_string) != SyncClient::SyncResult::SYNC_OK )
                return false;

            dictionaries_synced = 0;
        }

        if( sync_client.SyncData(*syncable_data_repository, sync_params.sync_direction, SO::Empty_string) == SyncClient::SyncResult::SYNC_OK )
            ++(*dictionaries_synced);
    }

    if( dictionaries_synced.has_value() )
        sync_client.Disconnect();

    return dictionaries_synced.value_or(0);
}
