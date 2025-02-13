#include "stdafx.h"
#include "SyncPutResponse.h"

SyncPutResponse::SyncPutResponse(SyncPutResult result, CString serverRevision)  :
    m_result(result),
    m_serverRevision(serverRevision)
{
}

SyncPutResponse::SyncPutResult SyncPutResponse::getResult() const
{
    return m_result;
}

CString SyncPutResponse::getServerRevision() const
{
    return m_serverRevision;
}
