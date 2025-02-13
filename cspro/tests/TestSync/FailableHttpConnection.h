#pragma once

#include <zNetwork/CurlHttpConnection.h>

class FailableHttpConnection : public CurlHttpConnection
{
public:

    FailableHttpConnection() :
        m_callCountGet(0),
        m_errorInGetAfterCalls(-1),
        m_callCountPost(0),
        m_errorInPostAfterCalls(-1)
    {}

    HttpResponse Request(const HttpRequest& request) override
    {
        if (request.method == HttpRequestMethod::HTTP_GET) {
            if (m_callCountGet++ == m_errorInGetAfterCalls)
                return HttpResponse{ m_errorInGetCode };
        }
        else if (request.method == HttpRequestMethod::HTTP_POST) {
            if (m_callCountPost++ == m_errorInPostAfterCalls)
                return HttpResponse{ m_errorInPostCode };
        }
        return CurlHttpConnection::Request(request);
    }

    void setErrorInGetAfterCalls(int errorCode, int numCalls)
    {
        m_errorInGetCode = errorCode;
        m_errorInGetAfterCalls = numCalls;
        m_callCountGet = 0;
    }

    void setErrorInPostAfterCalls(int errorCode, int numCalls)
    {
        m_errorInPostCode = errorCode;
        m_errorInPostAfterCalls = numCalls;
        m_callCountPost = 0;
    }

private:

    int m_errorInGetAfterCalls;
    int m_errorInGetCode;
    int m_callCountGet;
    int m_errorInPostAfterCalls;
    int m_errorInPostCode;
    int m_callCountPost;
};
