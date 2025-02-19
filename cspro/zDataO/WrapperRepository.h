#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepository.h>
#include <zToolsO/PointerClasses.h>


class ZDATAO_API WrapperRepository : public DataRepository
{
protected:
    WrapperRepository(cs::non_null_shared_or_raw_ptr<DataRepository> repository);

    void Open(DataRepositoryOpenFlag open_flag) override;

public:
    DataRepository& GetRealRepository() override;
    ISyncableDataRepository* GetSyncableDataRepository() override;
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
    void DeleteCase(const std::string& key) override;
    size_t GetNumberCases() override;
    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
                                                 std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
                                                 const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;
    void StartTransaction() override;
    void EndTransaction() override;

protected:
    cs::non_null_shared_or_raw_ptr<DataRepository> m_repository;
};
