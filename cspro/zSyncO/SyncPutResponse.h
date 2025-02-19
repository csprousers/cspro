#pragma once


class SyncPutResponse
{
public:
    enum class SyncPutResult
    {
        Complete,
        RevisionNotFound,
    };

    SyncPutResponse(SyncPutResult result, std::string server_revision = std::string());

    SyncPutResult GetResult() const              { return m_result; }
    const std::string& GetServerRevision() const { return m_serverRevision; }

private:
    SyncPutResult m_result;
    std::string m_serverRevision;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SyncPutResponse::SyncPutResponse(const SyncPutResult result, std::string server_revision/* = std::string()*/)
    :   m_result(result),
        m_serverRevision(std::move(server_revision))
{
}
