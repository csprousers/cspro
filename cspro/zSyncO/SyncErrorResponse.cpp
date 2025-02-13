#include "stdafx.h"
#include "SyncErrorResponse.h"

SyncErrorResponse::SyncErrorResponse(CString errorDescription) :
    m_errorDescription(errorDescription)
{
}

SyncErrorResponse::SyncErrorResponse(CString error, CString errorDescription) :
    m_error(error),
    m_errorDescription(errorDescription)
{
}

const CString& SyncErrorResponse::GetError() const
{
    return m_error;
}

const CString& SyncErrorResponse::GetErrorDescription() const
{
    return m_errorDescription;
}
