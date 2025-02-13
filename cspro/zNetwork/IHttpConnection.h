#pragma once

#include <zNetwork/HeaderList.h>
#include <zNetwork/ObservableResponseBody.h>
#include <istream>

struct ISyncListener;


enum class HttpRequestMethod { HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE };

inline const char* HttpRequestMethodToString(HttpRequestMethod method)
{
    switch (method) {
    case HttpRequestMethod::HTTP_POST:
        return "POST";
    case HttpRequestMethod::HTTP_PUT:
        return "PUT";
    case HttpRequestMethod::HTTP_DELETE:
        return "DELETE";
    case HttpRequestMethod::HTTP_GET:
    default:
        return "GET";
    }
}


struct HttpRequest
{
    CString url;
    HttpRequestMethod method = HttpRequestMethod::HTTP_GET;
    HeaderList headers;
    std::istream* upload_data = nullptr;
    int64_t upload_data_size_bytes = -1;
};


class HttpRequestBuilder
{
public:
    HttpRequestBuilder(const CString& url)
    {
        m_request.url = url;
    }

    HttpRequestBuilder& headers(const HeaderList& headers)
    {
        m_request.headers = headers;
        return *this;
    }

    HttpRequestBuilder& headers(const std::initializer_list<CString>& headers)
    {
        for (auto h : headers) {
            m_request.headers.push_back(h);
        }
        return *this;
    }

    HttpRequestBuilder& del()
    {
        m_request.method = HttpRequestMethod::HTTP_DELETE;
        return *this;
    }

    HttpRequestBuilder& post(std::istream& is, int64_t size_bytes = -1)
    {
        m_request.method = HttpRequestMethod::HTTP_POST;
        m_request.upload_data = &is;
        m_request.upload_data_size_bytes = size_bytes;
        return *this;
    }

    HttpRequestBuilder& put(std::istream& is, int64_t size_bytes = -1)
    {
        m_request.method = HttpRequestMethod::HTTP_PUT;
        m_request.upload_data = &is;
        m_request.upload_data_size_bytes = size_bytes;
        return *this;
    }

    HttpRequest& build()
    {
        return m_request;
    }

private:
    HttpRequest m_request;
};


struct HttpResponse
{
    HttpResponse(int status)
        : http_status(status)
    {}

    HttpResponse(int status, const HeaderList& headers)
        : http_status(status),
          headers(headers)
    {}

    int http_status;
    HeaderList headers;
    ObservableResponseBody body;
};


/**
* Communicate with http server by
* sending get, put, post and delete requests.
*/
struct IHttpConnection
{
    virtual ~IHttpConnection() {}

    virtual HttpResponse Request(const HttpRequest& request) = 0;

    virtual void setListener(ISyncListener* listener) = 0;
};
