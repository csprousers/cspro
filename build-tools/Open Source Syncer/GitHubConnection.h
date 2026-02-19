#pragma once

#include "GitHubJsonKeys.h"
#include <zNetwork/CurlHttpConnection.h>

class MemoryStream;


class GitHubConnection
{
public:
    GitHubConnection();
    ~GitHubConnection();

    // Returns a URL to access the GitHub API, optionally defaulting to csprousers/cspro.
    static std::string CreateApiUrl(cs::string_sz owner, cs::string_sz repo, cs::string_sz path);
    static std::string CreateApiUrl(cs::string_sz path) { return CreateApiUrl("csprousers", "cspro", path); }

    // Returns a URL to access the GitHub uploads API, optionally defaulting to csprousers/cspro.
    static std::string CreateUploadUrl(cs::string_sz owner, cs::string_sz repo, cs::string_sz path);
    static std::string CreateUploadUrl(cs::string_sz path) { return CreateUploadUrl("csprousers", "cspro", path); }

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

    // Sends a request with a JSON body using POST.
    HttpResponse PostJsonWithAuthentication(const std::string& url, const std::string& json_text);

    // Sends a request with a JSON body using PATCH.
    HttpResponse PatchJsonWithAuthentication(const std::string& url, const std::string& json_text);

    // Sends a request with a binary body using POST.
    HttpResponse PostBinaryWithAuthentication(const std::string& url, const BinaryBlock& binary_data);

private:
    // Creates headers for a call to the REST API.
    // If AcceptT is JsonNode, this will be added: Accept: application/vnd.github+json
    template<typename AcceptT>
    HeaderList CreateHeaders(bool requires_authentication);

    static std::optional<std::string> GetPaginatedNextLink(const std::string& link_header);

    HttpResponse RequestWithAuthentication(HttpRequestMethod method, const std::string& url,
                                           std::unique_ptr<MemoryStream> memory_stream, bool body_is_json);

private:
    std::unique_ptr<CurlHttpConnection> m_connection;
    std::string m_githubPAT;
};
