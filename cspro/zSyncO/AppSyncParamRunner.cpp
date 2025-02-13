#include "stdafx.h"
#include "AppSyncParamRunner.h"
#include "SyncClient.h"
#include "ISyncListener.h"


AppSyncParamRunner::AppSyncParamRunner(SyncClient* pClient)
    :   m_pClient(pClient)
{
}


bool AppSyncParamRunner::Run(const AppSyncParameters& params, DataRepository& mainDataFileRepo, ILoginDialog* pLoginDialog,
                             IDropboxAuthDialog* pDropboxAuthDialog, ICredentialStore* pCredentialStore)
{
    ISyncableDataRepository* pSyncableRepo = mainDataFileRepo.GetSyncableDataRepository();

    if (pSyncableRepo == nullptr) {
        m_pClient->getListener()->onError(100116, _T("syncdata"), (LPCTSTR)mainDataFileRepo.GetCaseAccess()->GetDataDict().GetName());
        return false;
    }

    if (params.server == _T("Dropbox")) {
        if (m_pClient->connectDropbox(pDropboxAuthDialog, pCredentialStore) != SyncClient::SyncResult::SYNC_OK)
            return false;
    } else if (SO::StartsWithNoCase(params.server, _T("http"))) {
        if (m_pClient->connectWeb(WS2CS(params.server), pLoginDialog, pCredentialStore) != SyncClient::SyncResult::SYNC_OK)
            return false;
    } else if (SO::StartsWithNoCase(params.server, _T("ftp"))) {
        if (m_pClient->connectFtp(WS2CS(params.server), pLoginDialog, pCredentialStore) != SyncClient::SyncResult::SYNC_OK)
            return false;
    }

    bool success = ( m_pClient->syncData(params.sync_direction, *pSyncableRepo, CString()) == SyncClient::SyncResult::SYNC_OK );

    m_pClient->disconnect();

    return success;
}
