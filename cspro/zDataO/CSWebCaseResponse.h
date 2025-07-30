#pragma once


// --------------------------------------------------------------------------
// CSWebCaseQuery
// --------------------------------------------------------------------------

enum class CSWebCaseQuery { identifiers, summaries, cases };


// --------------------------------------------------------------------------
// CSWebCaseResponse
// --------------------------------------------------------------------------

template<CSWebCaseQuery query>
struct CSWebCaseResponse
{
    struct JsonNodesForCase { JsonNode case_json_node; JsonNode metadata_json_node; };

    using Response = std::conditional_t<query == CSWebCaseQuery::cases, JsonNodesForCase, JsonNode>;
    Response response;
};


// --------------------------------------------------------------------------
// CSWebCacheCaseResponse
// --------------------------------------------------------------------------

template<CSWebCaseQuery query>
struct CSWebCacheCaseResponse
{
    // - If the cache does not contain the case, the response is std::monostate.
    // - If the cache contains the case but with a server_revision value less than
    //   m_currentServerRevision, the response is OldCaseData with information about
    //   the case in the cache.
    // - If the cache contains the up-to-date case, the response is CSWebCaseResponse<query>.

    struct OldCaseData { int64_t server_revision; int64_t position; };

    using CacheResponse = std::variant<std::monostate, OldCaseData, CSWebCaseResponse<query>>;
    CacheResponse response;
};
