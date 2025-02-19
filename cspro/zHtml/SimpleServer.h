#pragma once

#include <zHtml/zHtml.h>
#include <regex>
#include <thread>

namespace httplib { struct Request; struct Response; class Server; }


// --------------------------------------------------------------------------
// SimpleServer
//
// The SimpleServer class creates a local web server (on Windows) to listen
// to and respond to requests. The server's lifetime matches the object's.
//
// The server will be run on a different port than that used by
// LocalFileServer and SharedHtmlLocalFileServer.
// --------------------------------------------------------------------------

class ZHTML_API SimpleServer
{
public:
    // Starts the server.
    SimpleServer();

    // Stops the server.
    ~SimpleServer();

    // Returns the server's base URL, which will look like http://localhost:<port>/
    const std::string& GetBaseUrl() const { return m_baseUrl; }

    // Adds a mapping for the pattern with a callback function that takes a Handler to examine the request and return a response.
    class Handler;
    void AddMapping(const std::string& pattern, std::function<void(Handler&)> callback_function);

private:
    std::unique_ptr<httplib::Server> m_server;
    int m_port;
    std::thread m_thread;
    std::string m_baseUrl;
};


class ZHTML_API SimpleServer::Handler
{
public:
    Handler(const httplib::Request& request, httplib::Response& response);

    // Request handlers
    const std::string& GetRequestTarget();

    const std::smatch& GetRequestMatches();

    // Response handlers
    void SetResponseContent(const void* content_data, size_t content_size, cs::string_sz content_type);

    void SetResponseRedirect(const std::string& url);

private:
    const httplib::Request& m_request;
    httplib::Response& m_response;
};
