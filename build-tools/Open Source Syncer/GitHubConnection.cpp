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

    else if constexpr(std::is_same_v<T, HttpResponse>)
    {
        return response;
    }

    else
    {
        static_assert(std::is_same_v<T, std::string>);
        return response.body.ToString();
    }
}

template JsonNode GitHubConnection::Request(const std::string& url, bool requires_authentication/* = false*/);
template HttpResponse GitHubConnection::Request(const std::string& url, bool requires_authentication/* = false*/);
template std::string GitHubConnection::Request(const std::string& url, bool requires_authentication/* = false*/);


template<typename T>
T GitHubConnection::RequestWithPagination(const std::string& url, const bool requires_authentication)
{
    std::string json_array_text;
    bool end_json_array = true;

    cs::cref_optional<std::string> next_url = url;

    while( next_url.has_value() )
    {
        const HttpResponse response = Request<HttpResponse>(*next_url, requires_authentication);
        const std::string link_header = response.headers.GetValue("Link");
        std::string json_text = response.body.ToString();

        // if there was never a link header, we can return the result directly
        if( json_array_text.empty() && link_header.empty() )
        {
            json_array_text = std::move(json_text);
            end_json_array = false;
            break;
        }

        // otherwise strip the array characters and add it to the full JSON array
        SO::MakeTrim(json_text);

        if( json_text.size() < 2 || json_text.front() != '[' || json_text.back() != ']' )
            throw CSProException("Response is not a JSON array: " + url);

        json_array_text.push_back(json_array_text.empty() ? '[' : ',');
        json_array_text.append(std::string_view(json_text).substr(1, json_text.size() - 2));

        // process the potential next URL
        next_url = GetPaginatedNextLink(link_header);
    }

    ASSERT(!json_array_text.empty());

    if( end_json_array )
        json_array_text.push_back(']');

    if constexpr(std::is_same_v<T, std::string>)
    {
        return json_array_text;
    }

    else
    {
        JsonNode json_node = Json::Parse(json_array_text);

        if constexpr(std::is_same_v<T, JsonNode>)
        {
            return json_node;
        }

        else
        {
            static_assert(std::is_same_v<T, JsonNodeArray>);
            return json_node.GetArray();
        }
    }
}

template std::string GitHubConnection::RequestWithPagination(const std::string& url, bool requires_authentication);
template JsonNode GitHubConnection::RequestWithPagination(const std::string& url, bool requires_authentication);
template JsonNodeArray GitHubConnection::RequestWithPagination(const std::string& url, bool requires_authentication);


std::optional<std::string> GitHubConnection::GetPaginatedNextLink(const std::string& link_header)
{
    // matches: <url>; rel="value"
    const std::regex regex("<([^>]+)>\\s*;\\s*rel=\"([^\"]+)");

    auto begin = std::sregex_iterator(link_header.cbegin(), link_header.cend(), regex);
    auto end = std::sregex_iterator();

    for( auto itr = begin; itr != end; ++itr)
    {
        const std::string rel = (*itr)[2].str();

        if( rel == "next" )
            return (*itr)[1].str();
    }

    return std::nullopt;
}
