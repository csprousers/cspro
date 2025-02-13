#pragma once
#include <zSyncO/zSyncO.h>

class SYNC_API SyncPutResponse
{
public:

    enum class SyncPutResult
    {
        Complete,
        RevisionNotFound,
    };

    SyncPutResponse(SyncPutResult result, CString serverRevision = CString());

    SyncPutResult getResult() const;
    CString getServerRevision() const;

private:

    SyncPutResult m_result;
    CString m_serverRevision;
};
