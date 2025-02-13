#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/IHttpConnection.h>

class CurlOperationState;

/**
* Communicate with http server by
* sending get, put, post and delete requests.
*
* get, post, put and delete throw SyncException on connection errors
* but if the connection succeeds and returns an http result then
* that http result code is returned and no exception is thrown
* even if the http code indicates an error on the server (404, 500...)
*/


class ZNETWORK_API CurlHttpConnection : public IHttpConnection
{
public:
    CurlHttpConnection();

    ~CurlHttpConnection() override;

    HttpResponse Request(const HttpRequest& request) override;

    void setListener(ISyncListener* listener) override
    {
        m_listener = listener;
    }

private:

    void SetupEasyHandle();
    void Cleanup();
    void CheckForErrors();
    bool RunLoop();
    void* m_multi_handle;
    std::unique_ptr<CurlOperationState> m_state;
    ISyncListener* m_listener;
};
