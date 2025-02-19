#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/HttpConnection.h>

class CurlOperationState;


// --------------------------------------------------------------------------
// CurlHttpConnection
//
// Communicate with http server by sending get, put, post and delete
// requests.
//
// get, post, put and delete throw SyncException on connection errors
// but if the connection succeeds and returns an http result then
// that http result code is returned and no exception is thrown
// even if the http code indicates an error on the server (404, 500...)
// --------------------------------------------------------------------------

class ZNETWORK_API CurlHttpConnection : public HttpConnection
{
public:
    CurlHttpConnection();
    ~CurlHttpConnection();

    HttpResponse Request(const HttpRequest& request) override;

    std::shared_ptr<SyncListener> GetSharedSyncListener() override             { return m_syncListener; }
    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) override { m_syncListener = std::move(sync_listener); }

private:
    void SetupEasyHandle(CurlOperationState& state) const;
    void Cleanup(CurlOperationState& state) const;
    bool RunLoop(CurlOperationState& state) const;

private:
    std::shared_ptr<SyncListener> m_syncListener;
    void* m_multiHandle;

    std::mutex m_statesMutex;
    std::vector<std::unique_ptr<CurlOperationState>> m_states;
};
