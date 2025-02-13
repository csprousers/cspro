#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/WrapperRepository.h>

namespace Paradata
{
    class Event;
    class IParadataDriver;
    class NamedObject;
}


class ZDATAO_API ParadataWrapperRepository : public WrapperRepository
{
    friend class ParadataWrapperRepositoryCaseIterator;

public:
    ParadataWrapperRepository(std::shared_ptr<DataRepository> repository, DataRepositoryAccess access_type,
        Paradata::IParadataDriver& paradata_driver, std::shared_ptr<Paradata::NamedObject> paradata_dictionary_object);

    void Close() override;
    void ReadCase(Case& data_case, const CString& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override;
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
        std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order, 
        const CaseIteratorParameters* start_parameters = nullptr, size_t offset = 0, size_t limit = SIZE_MAX) override;

private:
    void Open(DataRepositoryOpenFlag open_flag) override;

    Paradata::IParadataDriver& m_paradataDriver;
    std::shared_ptr<Paradata::NamedObject> m_paradataDictionaryObject;

    void LogEvent(std::shared_ptr<Paradata::Event> event);
    void CreateCaseNotFoundEvent(const CString& key = CString());
};
