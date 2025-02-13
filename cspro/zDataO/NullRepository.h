#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepository.h>


class ZDATAO_API NullRepository : public DataRepository
{
public:
    NullRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type);
    ~NullRepository();

    void ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access) override;
    void Close() override;
    void DeleteRepository() override;
    bool ContainsCase(const CString& key) const override;
    void PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const override;
    std::optional<CaseKey> FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
	    const CaseIteratorParameters* start_parameters = nullptr) const override;
    void ReadCase(Case& data_case, const CString& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override;
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    size_t GetNumberCases() const override;
    size_t GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters = nullptr) const override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
        std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order, 
        const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;

private:
    void Open(DataRepositoryOpenFlag open_flag) override;
};
