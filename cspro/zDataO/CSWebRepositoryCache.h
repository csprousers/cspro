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
// No public methods throw exceptions other than:
//     - Create
//     - RetrieveCase
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
    void CacheQuery(std::string_view arguments_json_text_sv, const JsonNode& json_node) noexcept;

    // Returns the cached response to a query, or std::nullopt if not in the cache.
    std::optional<JsonNode> RetrieveQuery(std::string_view arguments_json_text_sv) noexcept;

    // Writes CSWeb's response to a case query.
    // In addition to the query itself, each case will be written to the cache.
    // The case cache will not be updated if what is currently present has more details
    // about the case (e.g., updating with an identifier when a case is already present).
    // If the case is up-to-date, but with a server_revision lower than the current one,
    // the server_revision will be updated.
    void CacheCaseQuery(std::string_view arguments_json_text_sv, const CSWebCaseQueryResponse& case_query_response) noexcept;

    // Returns the cached response to a case query.
    // See the notes in CSWebCacheCaseResponse's declaration for details on the return value.
    CSWebCacheCaseResponse RetrieveCaseQuery(CSWebCaseQuery query, std::string_view arguments_json_text_sv) noexcept;

    // Returns a cached case (or identifier or summary).
    // Exceptions are thrown on error.
    CSWebCaseResponse RetrieveCase(CSWebCaseQuery query, const CSWebCaseResponse& case_response);

    // Returns true if a non-deleted case with the given key exists in the cache.
    bool HasNonDeletedCaseByKey(const std::string& key) noexcept;

    // Writes binary data to the cache.
    void CacheBinaryData(const std::string& signature, const std::vector<std::byte>& content) noexcept;

    // Writes binary data from a sync operation to the cache.
    void CacheBinaryData(const SyncBinaryDataUploadManager& sync_binary_data_upload_manager) noexcept;

    // Returns the cached binary data, or std::nullopt if not in the cache.
    std::optional<std::vector<std::byte>> RetrieveBinaryData(const std::string& signature) noexcept;

    // Returns true if binary data with this signature exists in the cache.
    bool HasBinaryData(const std::string& signature) noexcept;

private:
    void Initialize(const JsonNode& dictionary_metadata_json_node);
    void CreateTablesAndIndices();
    void ClearOldQueries();

    bool IsServerRevisionKnown() noexcept;

    std::vector<int64_t> CacheCases(const CSWebCaseQueryResponse& case_query_response);
    void CacheCase(CSWebCaseQuery query, const CSWebCaseResponse& case_response);

    template<int json_column_number, int metadata_column_number>
    CSWebCaseResponse CreateCaseFromQuery(CSWebCaseQuery query, Sqlite::Statement& stmt);

    void WriteCasePositions(std::string_view arguments_json_text_sv, const std::vector<int64_t>& positions);
    std::optional<CSWebCacheStaleCaseData> RetrieveCasePositions(std::string_view arguments_json_text_sv);

private:
    CSWebRepository& m_repository;
    std::optional<int64_t> m_currentServerRevision;

    Sqlite::DB m_db;

    Sqlite::Statement m_stmtWriteQuery;
    Sqlite::Statement m_stmtReadQuery;

    Sqlite::Statement m_stmtWriteCase;
    Sqlite::Statement m_stmtUpdateCaseServerRevision;
    Sqlite::Statement m_stmtReadCase;
    Sqlite::Statement m_stmtReadCaseExistenceByQueryType;
    Sqlite::Statement m_stmtReadCaseExistenceNotDeletedByKey;

    Sqlite::Statement m_stmtWriteCasePositions;
    Sqlite::Statement m_stmtReadCasePositions;

    Sqlite::Statement m_stmtWriteBinaryData;
    Sqlite::Statement m_stmtReadBinaryData;
    Sqlite::Statement m_stmtHasBinaryData;
};
