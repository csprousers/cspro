#pragma once

#include <zHtml/zHtml.h>
#include <regex>


// --------------------------------------------------------------------------
// SimpleServer
//
// The SimpleServer class creates a local web server (on Windows) to listen
// to and respond to requests. The server's lifetime matches the object's.
//
// The server will be run on a different port than that used by
// LocalFileServer and SharedHtmlLocalFileServer.
//
// On Android, SimpleServer uses PortableLocalhost for its implementation.
// To use this version on Windows, define USE_PORTABLE_SIMPLE_SERVER.
// --------------------------------------------------------------------------

class SimpleServer
{
public:
    class Handler;
    class ImplInterface;

    // Starts the server.
    SimpleServer();

    // Stops the server.
    ~SimpleServer() { }

    // Returns the server's base URL with a trailing slash, which will look something like:
    //     http://localhost:<port>/
    //     https://appassets.androidplatform.net/lfs/1/
    const std::string& GetBaseUrl() const;

    // Adds a mapping for the pattern with a callback function that takes a Handler to examine the request and return a response.
    // The pattern must be a valid regular expression as it will be used by std::regex.
    void AddMapping(const std::string& pattern, std::function<void(Handler&)> callback_function);

private:
    ZHTML_API static std::unique_ptr<ImplInterface> CreateSimpleServer();

private:
    std::unique_ptr<ImplInterface> m_impl;
};


// --------------------------------------------------------------------------
// SimpleServer::ImplInterface
// --------------------------------------------------------------------------

class SimpleServer::ImplInterface
{
public:
    virtual ~ImplInterface() { }
    virtual const std::string& GetBaseUrl() const = 0;
    virtual void AddMapping(const std::string& pattern, std::function<void(Handler&)> callback_function) = 0;
};


// --------------------------------------------------------------------------
// SimpleServer::Handler
// --------------------------------------------------------------------------

class SimpleServer::Handler
{
public:
    virtual ~Handler() { }

    // Request handlers
    virtual const std::string& GetRequestTarget() = 0;

    virtual const std::smatch& GetRequestMatches() = 0;

    // Response handlers
    virtual void SetResponseContent(const void* content_data, size_t content_size, const std::string& content_type) = 0;

    virtual void SetResponseRedirect(const std::string& url) = 0;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SimpleServer::SimpleServer()
    :   m_impl(CreateSimpleServer())
{
    ASSERT(m_impl != nullptr);
}


inline const std::string& SimpleServer::GetBaseUrl() const
{
    return m_impl->GetBaseUrl();
}


inline void SimpleServer::AddMapping(const std::string& pattern, std::function<void(Handler&)> callback_function)
{
    m_impl->AddMapping(pattern, std::move(callback_function));
}
