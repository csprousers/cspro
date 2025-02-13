#pragma once


#include <zSyncO/zSyncO.h>

///<summary>
/// Error received from sync server
///</summary>
class SYNC_API SyncErrorResponse
{
public:

    explicit SyncErrorResponse(CString errorDescription);
    SyncErrorResponse(CString error, CString errorDescription);

    const CString& GetError() const;

    const CString& GetErrorDescription() const;

private:

    const CString m_error;
    const CString m_errorDescription;
};

