#include "stdafx.h"
#include "SimpleServer.h"
#include "LocalhostUrl.h"
#include <external/cpp-httplib/httplib.h>
#include <thread>


class HttplibSimpleServer : public SimpleServer::ImplInterface
{
public:
    HttplibSimpleServer();
    ~HttplibSimpleServer();

    const std::string& GetBaseUrl() const override { return m_baseUrl; }

    void AddMapping(const std::string& pattern, std::function<void(SimpleServer::Handler&)> callback_function) override;

private:
    std::unique_ptr<httplib::Server> m_server;
    int m_port;
    std::thread m_thread;
    std::string m_baseUrl;
};


class HttplibSimpleServerHandler : public SimpleServer::Handler
{
public:
    HttplibSimpleServerHandler(const httplib::Request& request, httplib::Response& response);

    const std::string& GetRequestTarget() override  { return m_request.target; }
    const std::smatch& GetRequestMatches() override { return m_request.matches; }

    void SetResponseContent(const void* content_data, size_t content_size, cs::string_sz content_type) override;
    void SetResponseRedirect(const std::string& url) override;

private:
    const httplib::Request& m_request;
    httplib::Response& m_response;
};



// --------------------------------------------------------------------------
// HttplibSimpleServer
// --------------------------------------------------------------------------

HttplibSimpleServer::HttplibSimpleServer()
    :   m_server(std::make_unique<httplib::Server>()),
        m_port(m_server->bind_to_any_port(LocalhostUrl::LocalhostHost)),
        m_thread([&]() { m_server->listen_after_bind(); }),
        m_baseUrl(FormatText("http://%s:%d/", LocalhostUrl::LocalhostHost, m_port))
{
}


HttplibSimpleServer::~HttplibSimpleServer()
{
    if( m_server->is_running() )
    {
        m_server->stop();
        m_thread.join();
    }
}


void HttplibSimpleServer::AddMapping(const std::string& pattern, std::function<void(SimpleServer::Handler&)> callback_function)
{
    m_server->Get(pattern,
        [user_callback_function = std::move(callback_function)](const httplib::Request& request, httplib::Response& response)
        {
            HttplibSimpleServerHandler handler(request, response);
            user_callback_function(handler);
        });
}



// --------------------------------------------------------------------------
// HttplibSimpleServerHandler
// --------------------------------------------------------------------------

HttplibSimpleServerHandler::HttplibSimpleServerHandler(const httplib::Request& request, httplib::Response& response)
    :   m_request(request),
        m_response(response)
{
}


void HttplibSimpleServerHandler::SetResponseContent(const void* const content_data, const size_t content_size, const cs::string_sz content_type)
{
    if( !content_type.empty() )
    {
        m_response.set_content(static_cast<const char*>(content_data), content_size, content_type.c_str());
    }

    else
    {
        m_response.set_content(static_cast<const char*>(content_data), content_size);
    }
}


void HttplibSimpleServerHandler::SetResponseRedirect(const std::string& url)
{
    m_response.set_redirect(url);
}



// --------------------------------------------------------------------------
// SimpleServer
// --------------------------------------------------------------------------

#ifndef USE_PORTABLE_SIMPLE_SERVER

std::unique_ptr<SimpleServer::ImplInterface> SimpleServer::CreateSimpleServer()
{
    return std::make_unique<HttplibSimpleServer>();
}

#endif
