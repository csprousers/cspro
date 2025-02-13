#include "stdafx.h"
#include "SyncGetResponse.h"
#include "CaseObservable.h"

SyncGetResponse::SyncGetResponse(SyncGetResult result) :
    m_result(result)
{}

SyncGetResponse::SyncGetResponse(SyncGetResult result, std::shared_ptr<CaseObservable> cases, CString serverRevision) :
    m_result(result),
    m_casesDownloaded(cases),
    m_serverRevision(serverRevision)
{
}

SyncGetResponse::SyncGetResponse(SyncGetResult result, std::shared_ptr<CaseObservable> cases, CString serverRevision, int total_cases) :
    m_result(result),
    m_casesDownloaded(cases),
    m_serverRevision(serverRevision),
    m_total_cases(total_cases)
{
}

SyncGetResponse::SyncGetResult SyncGetResponse::getResult() const
{
    return m_result;
}

CaseObservable* SyncGetResponse::getCases() const
{
    return m_casesDownloaded.get();
}

CString SyncGetResponse::getServerRevision() const
{
    return m_serverRevision;
}

std::optional<int> SyncGetResponse::getTotalCases() const
{
    return m_total_cases;
}
