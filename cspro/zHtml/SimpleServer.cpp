#include "stdafx.h"
#include "SimpleServer.h"
#include "LocalhostUrl.h"
#include <external/cpp-httplib/httplib.h>


SimpleServer::SimpleServer()
    :   m_server(std::make_unique<httplib::Server>()),
        m_port(m_server->bind_to_any_port(LocalhostUrl::LocalhostHost)),
        m_thread([&]() { m_server->listen_after_bind(); }),
        m_baseUrl(FormatText("http://%s:%d/", LocalhostUrl::LocalhostHost, m_port))
{
}


SimpleServer::~SimpleServer()
{
    if( m_server->is_running() )
    {
        m_server->stop();
        m_thread.join();
    }
}


void SimpleServer::AddMapping(const std::string& pattern, std::function<void(Handler&)> callback_function)
{
    m_server->Get(pattern,
        [user_callback_function = std::move(callback_function)](const httplib::Request& request, httplib::Response& response)
        {
            Handler handler(request, response);
            user_callback_function(handler);
        });
}


SimpleServer::Handler::Handler(const httplib::Request& request, httplib::Response& response)
    :   m_request(request),
        m_response(response)
{
}


const std::string& SimpleServer::Handler::GetRequestTarget()
{
    return m_request.target;
}


const std::smatch& SimpleServer::Handler::GetRequestMatches()
{
    return m_request.matches;
}


void SimpleServer::Handler::SetResponseContent(const void* const content_data, const size_t content_size, const cs::string_sz content_type)
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


void SimpleServer::Handler::SetResponseRedirect(const std::string& url)
{
    m_response.set_redirect(url);
}
