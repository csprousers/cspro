#pragma once

#include "GitHubJsonKeys.h"
#include <zNetwork/CurlHttpConnection.h>

class MemoryStream;


// --------------------------------------------------------------------------
// GitHubConnection manages generic GitHub API calls.
//
// A subclass exists, GitHubRepositoryConnection, that provides additional
// functionality useful when working with a specific repository.
// --------------------------------------------------------------------------

class GitHubConnection
{
public:
    GitHubConnection();
    virtual ~GitHubConnection();

    // Returns a URL to access the GitHub API.
    static std::string CreateApiUrl(cs::string_sz owner, cs::string_sz repo, cs::string_sz path);

    // Returns a URL to access the GitHub uploads API.
    static std::string CreateUploadUrl(cs::string_sz owner, cs::string_sz repo, cs::string_sz path);

    // Returns a response from the given URL, potentially requring authentication.
    // If T is JsonNode, the response body is parsed as JSON and returned as a JsonNode.
    // Other options for T: HttpResponse and std::string.
    // Only when T is HttpResponse can throw_when_status_not_200_OK be set to false.
    template<typename T, bool throw_when_status_not_200_OK = true>
    T Request(const std::string& url, bool requires_authentication = false);

    template<typename T, bool throw_when_status_not_200_OK = true>
    T RequestWithAuthentication(const std::string& url) { return Request<T, throw_when_status_not_200_OK>(std::move(url), true); }

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



// --------------------------------------------------------------------------
// GitHubRepositoryConnection
// --------------------------------------------------------------------------

class GitHubRepositoryConnection : public GitHubConnection
{
public:
    GitHubRepositoryConnection(std::string owner, std::string repo);

    // Returns a URL to access the GitHub API.
    std::string CreateApiUrl(cs::string_sz path) const    { return GitHubConnection::CreateApiUrl(m_owner, m_repo, path); }

    // Returns a URL to access the GitHub uploads API.
    std::string CreateUploadUrl(cs::string_sz path) const { return GitHubConnection::CreateUploadUrl(m_owner, m_repo, path); }

    // Returns the commit SHA for the specified tag name.
    // If the tag does not exist, a blank string is returned.
    // Errors accessing the API are thrown as exceptions.
    std::string GetTag(const std::string& tag_name);

    // Creates a lightweight tag as a reference: refs/tags/[tag_name].
    // No error is returned if the tag already exists.
    void CreateTag(const std::string& tag_name, const std::string& commit_sha);

private:
    std::string m_owner;
    std::string m_repo;
};
