#pragma once

class CSWebCaseResponse;


// --------------------------------------------------------------------------
// CSWebCaseQuery
// --------------------------------------------------------------------------

enum class CSWebCaseQuery { identifiers, summaries, cases };


constexpr const char* GetCSWebContentKey(CSWebCaseQuery query)
{
    return ( query == CSWebCaseQuery::identifiers ) ? JK::identifiers :
           ( query == CSWebCaseQuery::summaries )   ? JK::summaries :
         /*( query == CSWebCaseQuery::cases )*/       JK::cases;
}



// --------------------------------------------------------------------------
// CSWebCaseQueryResponse
//
// A wrapper around the JSON array sent by CSWeb containing zero or more
// cases.
// --------------------------------------------------------------------------

class CSWebCaseQueryResponse
{
public:
    CSWebCaseQueryResponse(CSWebCaseQuery query, JsonNode response_json_node);

    CSWebCaseQuery GetQuery() const { return m_query; }

    const JsonNode& GetJsonNode() const { return m_responseJsonNode; }

    bool IsCaseResponse() const { return ( m_contentJsonNodeArray != m_metadataJsonNodeArray ); }

    size_t GetCaseCount() const { return m_contentJsonNodeArray->size(); }

    CSWebCaseResponse GetCase(size_t index) const;

private:
    CSWebCaseQuery m_query;
    JsonNode m_responseJsonNode;
    std::shared_ptr<JsonNodeArray> m_contentJsonNodeArray;
    std::shared_ptr<JsonNodeArray> m_metadataJsonNodeArray;
};



// --------------------------------------------------------------------------
// CSWebCaseResponse
//
// A wrapper around the JSON object sent by CSWeb containing information on
// an identifier, summary, or case.
// --------------------------------------------------------------------------

class CSWebCaseResponse
{
public:
    CSWebCaseResponse(std::shared_ptr<JsonNode> content_json_node, std::shared_ptr<JsonNode> metadata_json_node);

    const JsonNode& GetContentJsonNode() const  { return *m_contentJsonNode; }
    const JsonNode& GetMetadataJsonNode() const { return *m_metadataJsonNode; }

    bool IsCaseResponse() const { return ( m_contentJsonNode != m_metadataJsonNode ); }

    bool IsContentEmpty() const { ASSERT(!m_contentJsonNode->IsEmpty() || IsCaseResponse());
                                  return m_contentJsonNode->IsEmpty(); }

    std::string GetKey() const { return m_contentJsonNode->Get<std::string>(JK::key); }

    std::string GetUuid() const { return m_contentJsonNode->Get<std::string>(JK::uuid); }

    template<typename T = int64_t>
    T GetPosition() const { return m_metadataJsonNode->Get<T>(JK::position); }

    int64_t GetRevision() const { return m_metadataJsonNode->Get<int64_t>(JK::revision); }

private:
    std::shared_ptr<JsonNode> m_contentJsonNode;
    std::shared_ptr<JsonNode> m_metadataJsonNode;
};



// --------------------------------------------------------------------------
// CSWebCacheStaleCaseData
//
// Information about potentially out-of-date cases that exist in the CSWeb
// cache.
// --------------------------------------------------------------------------

struct CSWebCacheStaleCaseData
{
    int64_t server_revision;
    std::vector<int64_t> positions;

    std::unique_ptr<std::string> CreateCacheHeaderJsonText() const;

    void WriteJson(JsonWriter& json_writer) const;
};



// --------------------------------------------------------------------------
// CSWebCacheCaseResponse
//
// If the cache does not contain the case(s), the response is std::monostate.
//
// If the cache contains the case(s) but recorded with a server revision less
// than m_currentServerRevision, the response is CSWebCacheStaleCaseData with
// information about the case(s) in the cache.
//
// If the cache contains the up-to-date case(s), the response is
// CSWebCaseQueryResponse.
// --------------------------------------------------------------------------

using CSWebCacheCaseResponse = std::variant<std::monostate,
                                            CSWebCacheStaleCaseData,
                                            CSWebCaseQueryResponse>;
