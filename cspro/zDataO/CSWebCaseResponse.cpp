#include "stdafx.h"
#include "CSWebCaseResponse.h"
#include "CSWebRepositoryJsonKeys.h"


// --------------------------------------------------------------------------
// CSWebCaseQueryResponse
// --------------------------------------------------------------------------

CSWebCaseQueryResponse::CSWebCaseQueryResponse(const CSWebCaseQuery query, JsonNode response_json_node)
    :   m_query(query),
        m_responseJsonNode(std::move(response_json_node))
{
    const char* const content_key = GetCSWebContentKey(query);

    ASSERT(response_json_node.IsObject());
    ASSERT(response_json_node.Contains(content_key) && response_json_node.Get(content_key).IsArray());

    m_contentJsonNodeArray = std::make_shared<JsonNodeArray>(response_json_node.GetArray(content_key));

    m_metadataJsonNodeArray = ( query == CSWebCaseQuery::cases ) ? std::make_shared<JsonNodeArray>(response_json_node.GetArray(JK::metadata)) :
                                                                   m_contentJsonNodeArray;

    ASSERT(m_contentJsonNodeArray->size() == m_metadataJsonNodeArray->size());
}


CSWebCaseResponse CSWebCaseQueryResponse::GetCase(const size_t index) const
{
    ASSERT(index <= GetCaseCount());

    std::shared_ptr<JsonNode> content_json_node = std::make_shared<JsonNode>((*m_contentJsonNodeArray)[index]);

    std::shared_ptr<JsonNode> metadata_json_node =
        ( m_contentJsonNodeArray == m_metadataJsonNodeArray ) ? content_json_node :
                                                                std::make_shared<JsonNode>((*m_metadataJsonNodeArray)[index]);

    return CSWebCaseResponse(std::move(content_json_node), std::move(metadata_json_node));
}



// --------------------------------------------------------------------------
// CSWebCaseResponse
// --------------------------------------------------------------------------

CSWebCaseResponse::CSWebCaseResponse(std::shared_ptr<JsonNode> content_json_node, std::shared_ptr<JsonNode> metadata_json_node)
    :   m_contentJsonNode(std::move(content_json_node)),
        m_metadataJsonNode(std::move(metadata_json_node))
{
    ASSERT(m_contentJsonNode != nullptr && m_contentJsonNode->IsObject());
    ASSERT(m_metadataJsonNode != nullptr && m_metadataJsonNode->IsObject());
}



// --------------------------------------------------------------------------
// CSWebCacheStaleCaseData
// --------------------------------------------------------------------------

std::unique_ptr<std::string> CSWebCacheStaleCaseData::CreateCacheHeaderJsonText() const
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::exclude, *this)
                .EndObject();

    return std::make_unique<std::string>(json_writer->ReleaseString());
}


void CSWebCacheStaleCaseData::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::serverRevision, server_revision)
               .Write(JK::positions,positions)
               .EndObject();
}
