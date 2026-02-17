#pragma once

class CurlHttpConnection;


class GitHubConnection
{
public:
    GitHubConnection();
    ~GitHubConnection();

    // Returns a response from the given URL, potentially requring authentication.
    // If T is JsonNode, the response body is parsed as JSON and returned as a JsonNode.
    template<typename T>
    T Request(const std::string& url, bool requires_authentication = false);

    template<typename T>
    T RequestWithAuthentication(const std::string& url) { return Request<T>(std::move(url), true); }

private:
    std::unique_ptr<CurlHttpConnection> m_connection;
    std::string m_githubPAT;
};
