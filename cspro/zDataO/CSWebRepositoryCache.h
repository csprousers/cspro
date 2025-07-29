#pragma once

#include <zDataO/CSWebRepository.h>
#include <zSql/DB.h>

struct CSWebRepositoryCacheCredential;
struct CSWebUser;


// --------------------------------------------------------------------------
// CSWebRepositoryCache
//
// This class caches data retrieved from CSWeb while using the CSWeb data
// source. The routines err on the side of caution, clearing the cache more
// frequently than necessary. These routines can be improved in the future.
//
// No public methods, other than Create, throw exceptions.
// --------------------------------------------------------------------------

class CSWebRepositoryCache
{
private:
    CSWebRepositoryCache(CSWebRepository& repository, const CSWebRepositoryCacheCredential& credential,
                         const JsonNode& dictionary_metadata_json_node);

public:
    ~CSWebRepositoryCache();

    // Returns an instance if the cache is successfully created or opened.
    // The current cache is cleared if the revisions on the server differ from the existing cache.
    // Exceptions are thrown on error.
    static std::unique_ptr<CSWebRepositoryCache> Create(CSWebRepository& repository, const DeviceId& server_device_id,
                                                        const CSWebUser& user, const JsonNode& dictionary_metadata_json_node);

    // Deletes the cache file and resets the unique pointer.
    static void DeleteCache(std::unique_ptr<CSWebRepositoryCache>& cache) noexcept;

    // Marks the cache as dirty (due to a write operation).
    void MarkCacheDirty() noexcept { m_currentServerRevision.reset(); }

    // Writes CSWeb's response to a query to the cache.
    void CacheQuery(std::string_view arguments_json_text_sv, const JsonNode& count_json_node) noexcept;

    // Returns the cached response to a query, or std::nullopt if not in the cache.
    std::optional<JsonNode> RetrieveQuery(std::string_view arguments_json_text_sv) noexcept;

    // Writes CSWeb's representation of a case to the cache.
    void CacheCase(const JsonNode& case_json_node, const JsonNode& metadata_json_node) noexcept;

    // Writes CSWeb's representation of an identifier or summary to the cache.
    // The cache will not be updated if what is present has more details about the case
    // (e.g., updating with an identifier when a case is already present).
    void CacheIdentifierOrSummary(const JsonNode& identifiers_or_summaries_json_node, CaseIterationContent content) noexcept;

    // Writes CSWeb's response to a single case query that returned case JSON (calling CacheCase).
    void CacheSingleCaseQuery(std::string_view arguments_json_text_sv, const JsonNode& case_json_node,
                              const JsonNode& metadata_json_node) noexcept;

    // Writes CSWeb's response to a single case query that returned identifier JSON (calling CacheIdentifierOrSummary).
    void CacheSingleIdentifierQuery(std::string_view arguments_json_text_sv, const JsonNode& identifiers_json_node) noexcept;

    // Returns the cached response to a single case query, or std::nullopt if not in the cache.
    // If metadata_json_node is non-null, the response will only be returned if it represents a full case.
    // If metadata_json_node is null, the response will be returned even if the cache only contains the data
    // for identifiers (not summaries), so this should not be used for summaries.
    std::optional<JsonNode> RetrieveSingleCaseQuery(std::string_view arguments_json_text_sv,
                                                    std::optional<JsonNode>* metadata_json_node) noexcept;

    // Returns true if a non-deleted case with the given key exists in the cache.
    bool HasNonDeletedCaseByKey(const std::string& key) noexcept;

private:
    void Initialize(const JsonNode& dictionary_metadata_json_node);
    void CreateTablesAndIndices();
    void ClearOldQueries();

    bool IsServerRevisionKnown() noexcept;

    void CacheCase(CaseIterationContent content, int64_t position, const JsonNode& identifiers_or_summaries_or_case_json_node,
                   const JsonNode* metadata_json_node);

    void CacheSingleCaseQuery(std::string_view arguments_json_text_sv, CaseIterationContent content, int64_t position,
                              const JsonNode& identifiers_or_case_json_node, const JsonNode* metadata_json_node);

private:
    CSWebRepository& m_repository;
    std::optional<int64_t> m_currentServerRevision;

    Sqlite::DB m_db;

    Sqlite::Statement m_stmtWriteQuery;
    Sqlite::Statement m_stmtReadQuery;

    Sqlite::Statement m_stmtWriteCase;
    Sqlite::Statement m_stmtReadCaseExistenceByDataCompleteness;
    Sqlite::Statement m_stmtReadCaseExistenceNotDeletedByKey;

    Sqlite::Statement m_stmtWriteSingleCasePosition;
    Sqlite::Statement m_stmtReadSingleCasePosition;
};
