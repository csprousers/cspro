#pragma once

#include <zDataO/DataRepository.h>


class MemoryRepository : public DataRepository
{
public:
    MemoryRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);

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

private:
    void Open(DataRepositoryOpenFlag open_flag) override;

    Case& GetCase(double position_in_repository);

    std::optional<size_t> GetCaseIndex(const std::string& key) const;
    Case& GetCase(const std::string& key);

    const Case& GetCaseByUuid(const std::string& uuid) const;

    std::vector<size_t> GetFilteredIndices(CaseIterationCaseStatus case_status,
                                           std::optional<CaseIterationMethod> iteration_method,
                                           std::optional<CaseIterationOrder> iteration_order,
                                           const CaseIteratorParameters* start_parameters) const;
    void FilterIndices(std::vector<size_t>& indices, const std::function<bool(const Case&)>& filter_function) const;

private:
    std::vector<std::shared_ptr<Case>> m_cases;
};
