#pragma once

#include <zSyncO/zSyncO.h>
#include <zAppO/AppSyncParameters.h>

class DataRepository;
class SyncClient;


// Execute synchronization from app sync parameter block.

class SYNC_API AppSyncParamRunner
{
public:
    static int Run(SyncClient& sync_client, const AppSyncParameters& sync_params, const std::vector<DataRepository*>& data_repositories_to_sync);
};
