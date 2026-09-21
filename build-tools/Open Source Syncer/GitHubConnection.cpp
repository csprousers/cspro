#include "StdAfx.h"
#include "GitHubConnection.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/MemoryStream.h>


namespace
{
    constexpr std::string_view HeaderUserAgent_sv = "User-Agent: CSPro Open Source Syncer";
}


// --------------------------------------------------------------------------
// GitHubConnection
// --------------------------------------------------------------------------

GitHubConnection::GitHubConnection()
    :   m_connection(std::make_unique<CurlHttpConnection>())
{
}


GitHubConnection::~GitHubConnection()
{
}


std::string GitHubConnection::CreateApiUrl(const cs::string_sz owner, const cs::string_sz repo, const cs::string_sz path)
{
    return FormatText("https://api.github.com/repos/%s/%s/%s", owner.c_str(), repo.c_str(), path.c_str());
}


std::string GitHubConnection::CreateUploadUrl(const cs::string_sz owner, const cs::string_sz repo, const cs::string_sz path)
{
    return FormatText("https://uploads.github.com/repos/%s/%s/%s", owner.c_str(), repo.c_str(), path.c_str());
}


template<typename AcceptT>
HeaderList GitHubConnection::CreateHeaders(const bool requires_authentication)
{
    HeaderList headers;
    headers.Add(std::string(HeaderUserAgent_sv));

    if constexpr(std::is_same_v<AcceptT, JsonNode>)
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
                                     "Specify a GitHub PAT using the Settings dialog.");
            }
        }

        headers.Add("Authorization: Bearer " + m_githubPAT);
    }

    return headers;
}


template<typename T, bool throw_when_status_not_200_OK/* = true*/>
T GitHubConnection::Request(const std::string& url, const bool requires_authentication/* = false*/)
{
    const HttpRequest request = HttpRequestBuilder(url, CreateHeaders<T>(requires_authentication)).build();
    HttpResponse response = m_connection->Request(request);

    if( response.http_status != HttpResponse::Status_200_OK )
    {
        if constexpr(throw_when_status_not_200_OK)
        {
            throw CSProException("Error accessing: " + url);
        }

        else if constexpr(!std::is_same_v<T, HttpResponse>)
        {
            throw ProgrammingErrorException();
        }
    }

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


HttpResponse GitHubConnection::RequestWithAuthentication(const HttpRequestMethod method, const std::string& url,
                                                         const std::unique_ptr<MemoryStream> memory_stream,
                                                         const bool body_is_json)
{
    ASSERT(memory_stream != nullptr);

    HeaderList headers = CreateHeaders<JsonNode>(true);

    body_is_json ? headers.Add_ContentType_Json() :
                   headers.Add_ContentType_OctetStream();

    HttpRequestBuilder request_builder(url, std::move(headers));

    request_builder.request(method, *memory_stream, memory_stream->size());

    const HttpRequest request = request_builder.build();

    return m_connection->Request(request);
}


HttpResponse GitHubConnection::PostJsonWithAuthentication(const std::string& url, const std::string& json_text)
{
    return RequestWithAuthentication(HttpRequestMethod::HTTP_POST, url, std::make_unique<MemoryStream>(json_text), true);
}


HttpResponse GitHubConnection::PatchJsonWithAuthentication(const std::string& url, const std::string& json_text)
{
    return RequestWithAuthentication(HttpRequestMethod::HTTP_PATCH, url, std::make_unique<MemoryStream>(json_text), true);
}


HttpResponse GitHubConnection::PostBinaryWithAuthentication(const std::string& url, const BinaryBlock& binary_data)
{
    return RequestWithAuthentication(HttpRequestMethod::HTTP_POST, url, std::make_unique<MemoryStream>(binary_data), false);
}



// --------------------------------------------------------------------------
// GitHubRepositoryConnection
// --------------------------------------------------------------------------

GitHubRepositoryConnection::GitHubRepositoryConnection(std::string owner, std::string repo)
    :   m_owner(std::move(owner)),
        m_repo(std::move(repo))
{
    ASSERT(!m_owner.empty() && !m_repo.empty());
}


std::string GitHubRepositoryConnection::GetTag(const std::string& tag_name)
{
    const HttpResponse response = RequestWithAuthentication<HttpResponse, false>(
        CreateApiUrl("git/ref/tags/" + tag_name)
    );

    if( response.http_status == HttpResponse::Status_200_OK )
    {
        const JsonNode json_node = Json::Parse(response.body.ToString());

        return json_node.Get(JK::object)
                        .Get<std::string>(JK::sha);
    }

    else if( response.http_status == HttpResponse::Status_404_NotFound )
    {
        return std::string();
    }

    else
    {
        throw CSProException("Error querying a tag reference:\n\n" + response.body.ToString());
    }
}


void GitHubRepositoryConnection::CreateTag(const std::string& tag_name, const std::string& commit_sha)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::ref, "refs/tags/" + tag_name)
                .Write(JK::sha, commit_sha)
                .EndObject();

    const HttpResponse response = PostJsonWithAuthentication(
        CreateApiUrl("git/refs"),
        json_writer->ReleaseString()
    );

    if( response.http_status == HttpResponse::Status_200_OK )
        return;

    // 422 may be returned if the tag already exists, so verify that the tag exists
    if( response.http_status == HttpResponse::Status_422_UnprocessableContent )
    {
        try
        {
            if( commit_sha == GetTag(tag_name) )
                return;
        }
        catch(...) { } // in case of another error, throw the original error
    }

    throw CSProException("Error creating a tag reference, error: %d:\n\n%s",
                         response.http_status, response.body.ToString().c_str());
}


template<typename T/* = std::vector<GitHubRelease>*/>
T GitHubRepositoryConnection::GetReleases()
{
    std::string json_text = RequestWithPagination<std::string>(
        CreateApiUrl("releases"),
        true
    );

    if constexpr(std::is_same_v<T, std::vector<GitHubRelease>>)
    {
        return Json::Parse(json_text).GetArray().GetVector<GitHubRelease>();
    }

    else
    {
        return json_text;
    }
}

template std::vector<GitHubRelease> GitHubRepositoryConnection::GetReleases();
template std::string GitHubRepositoryConnection::GetReleases();


int64_t GitHubRepositoryConnection::CreateDraftRelease(const std::string& tag_name, const std::string& release_name,
                                                       std::string release_notes, const bool prerelease)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::tag_name, tag_name)
                .Write(JK::name, release_name)
                .Write(JK::body, SO::ToNewlineLF(std::move(release_notes)))
                .Write(JK::draft, true)
                .Write(JK::prerelease, prerelease)
                .EndObject();

    const HttpResponse response = PostJsonWithAuthentication(
        CreateApiUrl("releases"),
        json_writer->ReleaseString()
    );

    if( response.http_status != HttpResponse::Status_201_Created )
        throw CSProException("The draft release could not be created, error: %d", response.http_status);

    const JsonNode json_node = Json::Parse(response.body.ToString());

    // ensure that the upload URL is as expected
    const std::string upload_url = json_node.Get<std::string>(JK::upload_url);

    if( Path::GetFilename(upload_url) != "assets{?name,label}" )
        throw CSProException("Modify this tool to handle upload URLs in the form: " + upload_url);

    return json_node.Get<int64_t>(JK::id);
}


void GitHubRepositoryConnection::UploadReleaseAsset(const int64_t release_id, const std::string& filename, const BinaryBlock& data)
{
    const std::string path = FormatText("releases/" Formatter_int64_t "/assets?name=%s",
                                        release_id, Encoders::ToUriComponent(filename).c_str());

    const HttpResponse response = PostBinaryWithAuthentication(
        CreateUploadUrl(path),
        data
    );

    if( response.http_status != HttpResponse::Status_201_Created )
    {
        throw CSProException("The release asset '%s' could not be uploaded, error: %d",
                             filename.c_str(), response.http_status);
    }
}


void GitHubRepositoryConnection::PublishRelease(const int64_t release_id)
{
    const HttpResponse response = PatchJsonWithAuthentication(
        CreateApiUrl("releases/" + IntToString(release_id)),
        R"({"draft":false})"
    );

    if( response.http_status != HttpResponse::Status_200_OK )
        throw CSProException("The draft release could not published, error: %d", response.http_status);
}


int64_t GitHubRepositoryConnection::CreateRelease(const std::string& tag_name, const std::string& release_name,
                                                  std::string release_notes, const bool prerelease,
                                                  const std::vector<std::tuple<std::string, std::shared_ptr<const BinaryBlock>>>& assets)
{
    // create the release as a draft
    const int64_t release_id = CreateDraftRelease(tag_name, release_name, std::move(release_notes), prerelease);

    // upload the assets
    for( const auto& [filename, data] : assets )
    {
        ASSERT(filename == Path::CreateValidFilename(filename));
        ASSERT(data != nullptr);

        UploadReleaseAsset(release_id, filename, *data);
    }

    // toggle the draft flag, publishing the release
    PublishRelease(release_id);

    return release_id;
}
