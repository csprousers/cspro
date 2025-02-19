#pragma once

class CaseObservable;


class SyncGetResponse
{
public:
    enum class SyncGetResult
    {
        Complete,
        RevisionNotFound,
        MoreData
    };

    SyncGetResponse(SyncGetResult result);
    SyncGetResponse(SyncGetResult result, std::shared_ptr<CaseObservable> cases, std::string server_revision, std::optional<int> total_cases = std::nullopt);

    SyncGetResult GetResult() const              { return m_result; }
    CaseObservable* GetCases()                   { return m_casesDownloaded.get(); }
    const std::string& GetServerRevision() const { return m_serverRevision; }
    std::optional<int> GetTotalCases() const     { return m_totalCases; }

private:
    SyncGetResult m_result;
    std::shared_ptr<CaseObservable> m_casesDownloaded;
    std::string m_serverRevision;
    std::optional<int> m_totalCases;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SyncGetResponse::SyncGetResponse(const SyncGetResult result)
    :   m_result(result)
{
}


inline SyncGetResponse::SyncGetResponse::SyncGetResponse(const SyncGetResult result, std::shared_ptr<CaseObservable> cases,
                                                         std::string server_revision, std::optional<int> total_cases/* = std::nullopt*/)
    :   m_result(result),
        m_casesDownloaded(std::move(cases)),
        m_serverRevision(std::move(server_revision)),
        m_totalCases(std::move(total_cases))
{
}
