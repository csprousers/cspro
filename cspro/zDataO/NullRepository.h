#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepository.h>


class ZDATAO_API NullRepository : public DataRepository
{
public:
    NullRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);
    ~NullRepository();

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
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content,
                                                 const CaseIteratorSettings& iterator_settings,
                                                 size_t offset = 0, size_t limit = SIZE_MAX) override;

private:
    void Open(DataRepositoryOpenFlag open_flag) override;
};
