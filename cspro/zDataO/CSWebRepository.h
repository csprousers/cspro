#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepository.h>

class ConnectResponse;
class CSWebConnection;
class LoginCredentials;
class SyncBinaryDataUploadManager;
class SyncCaseSerializer;
class SyncErrorFormatter;


class ZDATAO_API CSWebRepository : public DataRepository
{
    friend class CSWebBinaryContentReader;
    friend class CSWebRepositoryIterator;

public:
    CSWebRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);
    ~CSWebRepository();

    void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) override;
    void ToggleReadWriteMode() override;
    void Close() override;
    void DeleteRepository() override;
    bool ContainsCase(const std::string& key) override;
    void PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository) override;
    DataRepositoryUniqueCaseIdentifer GetUniqueCaseIdentifer(const CaseKey& case_key) override;
    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
                                       const CaseIteratorParameters* start_parameters = nullptr) override;
    void ReadCase(Case& data_case, const std::string& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void ReadCaseByUuid(Case& data_case, const std::string& uuid) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override;
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    size_t GetNumberCases() override;
    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
                                                 std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
                                                 const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;

    static std::string CalculateDictionaryKeyStructure(const CDataDict& dictionary);

    static std::unique_ptr<SyncCaseSerializer> CreateSyncCaseSerializer(UniqueId repository_id,
                                                                        std::shared_ptr<const CaseAccess> case_access,
                                                                        std::shared_ptr<CSWebConnection> csweb_connection);

    static size_t ParseJsonCount(const JsonNode& json_node);
    static CaseKey ParseJsonIdentifier(const JsonNode& json_node);
    static CaseSummary ParseJsonSummary(const JsonNode& json_node);
    static void ParseJsonCase(Case& data_case, const JsonNode& case_json_node, const JsonNode& metadata_json_node, SyncCaseSerializer& sync_case_serializer);

    // Returns the dictionary from CSWeb using any credentials specified in the connection string.
    // Exceptions will be thrown on read errors.
    static std::unique_ptr<CDataDict> GetDictionary(const std::string& dictionary_name, const ConnectionString& connection_string);

private:
    static std::string CreateDeviceId();

    [[noreturn]] static void RethrowException(const std::exception& exception, const std::string& dictionary_name, std::unique_ptr<SyncErrorFormatter>& sync_error_formatter);
    [[noreturn]] void RethrowException(const std::exception& exception);

    std::string GetDictionaryNameForErrors() const;

    void ResetCaseObjects();

    void Open(DataRepositoryOpenFlag open_flag) override;

    static LoginCredentials CreateLoginCredentials(const ConnectionString& connection_string);

    JsonNode EnsureDictionaryExistsAndGetDictionaryMetadata(CSWebConnection& csweb_connection, DataRepositoryOpenFlag open_flag) const;
    static void PutDictionaryThatDoesNotExist(CSWebConnection& csweb_connection, const CDataDict& dictionary, const std::string& syncable_name);

    size_t ExecuteCaseCountQuery(std::string_view arguments_json_text_sv) const;

    template<bool requires_metadata = false, typename CF>
    void ExecuteSingleCaseQuery(const char* content, const char* status,
                                const char* filter_type, std::string_view filter_value_sv,
                                const CF& callback_function) const;

    // CSWeb has its own limit on the content entries it returns in one request,
    // so the specified limit may not be completely fulfilled in a single request.
    template<bool requires_metadata = false>
    std::string CreateKeySearchQuery(const char* content, CaseIterationCaseStatus case_status,
                                     std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
                                     const CaseIteratorParameters* start_parameters, size_t offset, size_t limit);

    void ReadCase(Case& data_case, const char* status, const char* filter_type, std::string_view filter_value_sv);

    std::unique_ptr<SyncBinaryDataUploadManager> CreateSyncBinaryDataUploadManager();

private:
    const DeviceId m_deviceId;
    std::string m_syncableDictionaryName;

    std::shared_ptr<CSWebConnection> m_cswebConnection;
    std::unique_ptr<const ConnectResponse> m_cswebConnectResponse;

    std::unique_ptr<SyncErrorFormatter> m_syncErrorFormatter;

    std::unique_ptr<SyncCaseSerializer> m_syncCaseSerializer;
    std::optional<std::unique_ptr<SyncBinaryDataUploadManager>> m_syncBinaryDataUploadManager; // null if not necessary

    std::unique_ptr<Case> m_temporaryCase;
};
