#pragma once

class CurlHttpConnection;


class GitHubConnection
{
public:
    GitHubConnection();
    ~GitHubConnection();

    // Returns a response from the given URL, potentially requring authentication.
    // If T is JsonNode, the response body is parsed as JSON and returned as a JsonNode.
    // Other options for T: HttpResponse and std::string.
    template<typename T>
    T Request(const std::string& url, bool requires_authentication = false);

    template<typename T>
    T RequestWithAuthentication(const std::string& url) { return Request<T>(std::move(url), true); }

    // Processes the "Link" response header to process all pages of a request.
    // The response is assumed to be a JSON array.
    // Options for T: std::string, JsonNode, JsonNodeArray.
    template<typename T>
    T RequestWithPagination(const std::string& url, bool requires_authentication);

private:
    static std::optional<std::string> GetPaginatedNextLink(const std::string& link_header);

private:
    std::unique_ptr<CurlHttpConnection> m_connection;
    std::string m_githubPAT;
};
