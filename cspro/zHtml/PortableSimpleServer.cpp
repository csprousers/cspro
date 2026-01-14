#include "stdafx.h"
#include "SimpleServer.h"
#include "PortableLocalhost.h"


class PortableSimpleServer : public SimpleServer::ImplInterface, public KeyBasedVirtualFileMappingHandler
{
public:
    PortableSimpleServer();

    // SimpleServer::ImplInterface overrides
    const std::string& GetBaseUrl() const override { return m_baseUrl; }

    void AddMapping(const std::string& pattern, std::function<void(SimpleServer::Handler&)> callback_function) override;

    // KeyBasedVirtualFileMappingHandler overrides
    bool ServeContent(VirtualFileMappingResponse& response, const std::string& key) override;

private:
    std::string m_baseUrl;
    std::vector<std::tuple<std::regex, std::function<void(SimpleServer::Handler&)>>> m_mappings;
};


class PortableSimpleServerHandler : public SimpleServer::Handler
{
public:
    PortableSimpleServerHandler(VirtualFileMappingResponse& response, const std::string& target, const std::smatch& matches);

    const std::string& GetRequestTarget() override  { return m_target; }
    const std::smatch& GetRequestMatches() override { return m_matches; }

    void SetResponseContent(const void* content_data, size_t content_size, const std::string& content_type) override;
    void SetResponseRedirect(const std::string& url) override;

private:
    VirtualFileMappingResponse& m_response;
    const std::string& m_target;
    const std::smatch& m_matches;
};



// --------------------------------------------------------------------------
// PortableSimpleServer
// --------------------------------------------------------------------------

PortableSimpleServer::PortableSimpleServer()
{
    PortableLocalhost::CreateVirtualDirectory(*this);

    m_baseUrl = PortableFunctions::PathEnsureTrailingForwardSlash(m_virtualFileMapping->GetUrl());
}


void PortableSimpleServer::AddMapping(const std::string& pattern, std::function<void(SimpleServer::Handler&)> callback_function)
{
    m_mappings.emplace_back(std::regex(pattern), std::move(callback_function));
}


bool PortableSimpleServer::ServeContent(VirtualFileMappingResponse& response, const std::string& key)
{
    // see if the request matches any of our handlers
    const std::string target = '/' + key;
    std::smatch matches;

    for( const auto& [pattern, mapping] : m_mappings )
    {
        if( std::regex_match(target, matches, pattern))
        {
            try
            {
                PortableSimpleServerHandler handler(response, target, matches);
                mapping(handler);
                return true;
            }
            catch(...) { }

            break;
        }
    }

    return false;
}



// --------------------------------------------------------------------------
// PortableSimpleServerHandler
// --------------------------------------------------------------------------

PortableSimpleServerHandler::PortableSimpleServerHandler(VirtualFileMappingResponse& response, const std::string& target, const std::smatch& matches)
    :   m_response(response),
        m_target(target),
        m_matches(matches)
{
}


void PortableSimpleServerHandler::SetResponseContent(const void* const content_data, const size_t content_size, const std::string& content_type)
{
    m_response.SetContent(content_data, content_size, content_type);
}


void PortableSimpleServerHandler::SetResponseRedirect(const std::string& /*url*/)
{
    // only implemented by HttplibSimpleServerHandler
    ASSERT(false);
}



// --------------------------------------------------------------------------
// SimpleServer
// --------------------------------------------------------------------------

#if !defined(WIN_DESKTOP) || defined(USE_PORTABLE_SIMPLE_SERVER)

std::unique_ptr<SimpleServer::ImplInterface> SimpleServer::CreateSimpleServer()
{
    return std::make_unique<PortableSimpleServer>();
}

#endif
