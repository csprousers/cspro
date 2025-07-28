#pragma once

#include <zDataO/CSWebRepository.h>
#include <zSql/DB.h>

struct CSWebRepositoryCacheCredential;
struct CSWebUser;
class SyncCaseV3JsonSerializer;


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
    enum class DataType { Count = 10, CaseKey = 20, CaseSummary = 30, Case = 40 };

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

    // Parses the case returned from CSWeb and writes it to the cache.
    void CacheCase(const JsonNode& case_json_node, const JsonNode& metadata_json_node) noexcept;

    // Returns true if a non-deleted case with the given key was retrieved from the cache.
    bool ReadCaseByKey(Case& data_case, const std::string& key) noexcept;

    // Returns true if a case with the given position was retrieved from the cache.
    bool ReadCaseByPosition(Case& data_case, int64_t position) noexcept;

private:
    void Initialize(const JsonNode& dictionary_metadata_json_node);

private:
    CSWebRepository& m_repository;
    std::optional<int64_t> m_currentServerRevision;

    std::shared_ptr<const CaseAccess> m_caseAccess;
    std::unique_ptr<Case> m_case;
    std::unique_ptr<SyncCaseV3JsonSerializer> m_syncCaseJsonSerializer;

    Sqlite::DB m_db;
    Sqlite::Statement m_stmtWriteCase;
    Sqlite::Statement m_stmtQueryCasesByKey;
    Sqlite::Statement m_stmtQueryCasesByPosition;
};
