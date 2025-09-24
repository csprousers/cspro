#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/HeaderList.h>
#include <zNetwork/ObservableResponseBody.h>
#include <istream>

class SyncListener;


enum class HttpRequestMethod { HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_DELETE };

constexpr const char* HttpRequestMethodToString(HttpRequestMethod method);


struct HttpRequest
{
    std::string url;
    HeaderList headers;
    HttpRequestMethod method = HttpRequestMethod::HTTP_GET;
    std::istream* upload_data = nullptr;
    int64_t upload_data_size_bytes = -1;
};


class HttpRequestBuilder
{
public:
    HttpRequestBuilder(std::string url);
    HttpRequestBuilder(std::string url, HeaderList headers);

    HttpRequestBuilder& headers(HeaderList headers);

    HttpRequestBuilder& del();

    HttpRequestBuilder& post(std::istream& is, int64_t size_bytes = -1);

    HttpRequestBuilder& put(std::istream& is, int64_t size_bytes = -1);

    HttpRequest build() { return std::move(m_request); }

private:
    HttpRequest m_request;
};


struct HttpResponse
{
    HttpResponse(int status);
    HttpResponse(int status, HeaderList headers_);

    static constexpr int Status_200_OK                  = 200;
    static constexpr int Status_206_PartialContent      = 206;
    static constexpr int Status_304_NotModified         = 304;
    static constexpr int Status_400_BadRequest          = 400;
    static constexpr int Status_401_Unauthorized        = 401;
    static constexpr int Status_403_Forbidden           = 403;
    static constexpr int Status_404_NotFound            = 404;
    static constexpr int Status_412_Precondition_Failed = 412;
    static constexpr int Status_429_TooManyRequests     = 429;
    static constexpr int Status_500_InternalServerError = 500;

    int http_status;
    HeaderList headers;
    ObservableResponseBody body;
};


// A class to communicate with a HTTP server by sending get, put, post and delete requests.

class HttpConnection
{
public:
    virtual ~HttpConnection() { }

    virtual HttpResponse Request(const HttpRequest& request) = 0;

    virtual std::shared_ptr<SyncListener> GetSharedSyncListener() = 0;
    virtual void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) = 0;

    // create a platform-specific HttpConnection
    ZNETWORK_API static std::unique_ptr<HttpConnection> Create();
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr const char* HttpRequestMethodToString(const HttpRequestMethod method)
{
    switch( method )
    {
        case HttpRequestMethod::HTTP_POST:   return "POST";
        case HttpRequestMethod::HTTP_PUT:    return "PUT";
        case HttpRequestMethod::HTTP_DELETE: return "DELETE";
        case HttpRequestMethod::HTTP_GET:
        default:                             return "GET";
    }
}


inline HttpRequestBuilder::HttpRequestBuilder(std::string url)
    :   m_request{ std::move(url) }
{
}


inline HttpRequestBuilder::HttpRequestBuilder(std::string url, HeaderList headers)
    :   m_request{ std::move(url), std::move(headers) }
{
}


inline HttpRequestBuilder& HttpRequestBuilder::headers(HeaderList headers)
{
    m_request.headers = std::move(headers);
    return *this;
}


inline HttpRequestBuilder& HttpRequestBuilder::del()
{
    m_request.method = HttpRequestMethod::HTTP_DELETE;
    return *this;
}


inline HttpRequestBuilder& HttpRequestBuilder::post(std::istream& is, const int64_t size_bytes/* = -1*/)
{
    m_request.method = HttpRequestMethod::HTTP_POST;
    m_request.upload_data = &is;
    m_request.upload_data_size_bytes = size_bytes;
    return *this;
}


inline HttpRequestBuilder& HttpRequestBuilder::put(std::istream& is, const int64_t size_bytes/* = -1*/)
{
    m_request.method = HttpRequestMethod::HTTP_PUT;
    m_request.upload_data = &is;
    m_request.upload_data_size_bytes = size_bytes;
    return *this;
}


inline HttpResponse::HttpResponse(const int status)
    :   http_status(status)
{
}


inline HttpResponse::HttpResponse(const int status, HeaderList headers_)
    :   http_status(status),
        headers(std::move(headers_))
{
}
