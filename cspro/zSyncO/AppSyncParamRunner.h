#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/SyncClient.h>
#include <zAppO/AppSyncParameters.h>
#include <zDataO/DataRepository.h>

struct ILoginDialog;
struct ICredentialStore;
struct IDropboxAuthDialog;


///<summary>
/// Execute synchronization from app sync parameter block.
///</summary>
class SYNC_API AppSyncParamRunner
{
public:
    AppSyncParamRunner(SyncClient* pSyncClient);

    bool Run(const AppSyncParameters& params, DataRepository& mainDataFileRepo, ILoginDialog* pLoginDlg,
             IDropboxAuthDialog* pDropboxAuthDialog, ICredentialStore* pCredentialStore);

private:
    SyncClient* m_pClient;
};
