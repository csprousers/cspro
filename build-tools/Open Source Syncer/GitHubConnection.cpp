#include "StdAfx.h"
#include "GitHubConnection.h"
#include <zNetwork/CurlHttpConnection.h>


namespace
{
    constexpr std::string_view HeaderUserAgent_sv = "User-Agent: CSPro Open Source Syncer";
}


GitHubConnection::GitHubConnection()
    :   m_connection(std::make_unique<CurlHttpConnection>())
{
}


GitHubConnection::~GitHubConnection()
{
}


template<typename T>
T GitHubConnection::Request(const std::string& url, const bool requires_authentication/* = false*/)
{
    HeaderList headers;
    headers.Add(std::string(HeaderUserAgent_sv));

    if constexpr(std::is_same_v<T, JsonNode>)
    {
        headers.Add("Accept: application/vnd.github+json");
    }

    if( requires_authentication )
    {
        if( m_githubPAT.empty() )
        {
            m_githubPAT = Controller::GetInstance().GetGitHubPAT();

            if( m_githubPAT.empty() )
            {
                throw CSProException("This request requires GitHub authentication. "
                                     "Specify a GitHub PAT using the Settings dialog.\n\n" + url);
            }
        }

        headers.Add("Authorization: Bearer " + m_githubPAT);
    }

    const HttpRequest request = HttpRequestBuilder(url, std::move(headers)).build();
    HttpResponse response = m_connection->Request(request);

    if( response.http_status != HttpResponse::Status_200_OK )
        throw CSProException("Error accessing: " + url);

    if constexpr(std::is_same_v<T, JsonNode>)
    {
        return Json::Parse(response.body.ToString());
    }

    else
    {
        static_assert(std::is_same_v<T, std::string>);
        return response.body.ToString();
    }
}

template JsonNode GitHubConnection::Request(const std::string& url, bool requires_authentication/* = false*/);
template std::string GitHubConnection::Request(const std::string& url, bool requires_authentication/* = false*/);
