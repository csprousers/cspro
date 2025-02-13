#pragma once
#include <zSyncO/zSyncO.h>

class Case;
struct CaseObservable;

class SYNC_API SyncGetResponse
{
public:

    enum class SyncGetResult
    {
        Complete,
        RevisionNotFound,
        MoreData
    };

    SyncGetResponse(SyncGetResult result);

    SyncGetResponse(SyncGetResult result,
        std::shared_ptr<CaseObservable> cases,
        CString serverRevision);

    SyncGetResponse(SyncGetResult result,
        std::shared_ptr<CaseObservable> cases,
        CString serverRevision,
        int total_cases);

    SyncGetResult getResult() const;
    CaseObservable* getCases() const;
    CString getServerRevision() const;
    std::optional<int> getTotalCases() const;

private:

    SyncGetResult m_result;
    std::shared_ptr<CaseObservable> m_casesDownloaded;
    CString m_serverRevision;
    std::optional<int> m_total_cases;
};
