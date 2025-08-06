#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/WrapperRepository.h>

namespace Paradata { class Event; class NamedObject; class ParadataDriver; }


class ZDATAO_API ParadataWrapperRepository : public WrapperRepository
{
    friend class ParadataWrapperRepositoryCaseIterator;

public:
    ParadataWrapperRepository(cs::non_null_shared_or_raw_ptr<DataRepository> repository,
                              cs::non_null_shared_or_raw_ptr<Paradata::ParadataDriver> paradata_driver,
                              std::shared_ptr<Paradata::NamedObject> paradata_dictionary_object);

protected:
    void Open(DataRepositoryOpenFlag open_flag) override;

public:
    void Close() override;
    void ReadCase(Case& data_case, const std::string& key) override;
    void ReadCase(Case& data_case, double position_in_repository) override;
    void ReadCaseByUuid(Case& data_case, const std::string& uuid) override;
    void WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter = nullptr) override;
    void DeleteCase(double position_in_repository, bool deleted = true) override;
    void DeleteCase(const std::string& key) override;
    std::unique_ptr<CaseIterator> CreateIterator(CaseIterationContent iteration_content,
                                                 const CaseIteratorSettings& iterator_settings,
                                                 size_t offset = 0, size_t limit = SIZE_MAX) override;

private:
    template<typename EventT, typename... Args>
    void LogEvent(Args&&... args);

    template<bool read_parameter_is_uuid, typename T>
    void ReadCaseWorker(Case& data_case, const T& read_parameter);

    template<typename... Args>
    void DeleteCaseWorker(std::string key, double position_in_repository, bool deleted, Args const&... delete_arguments);

private:
    const cs::non_null_shared_or_raw_ptr<Paradata::ParadataDriver> m_paradataDriver;
    const std::shared_ptr<Paradata::NamedObject> m_paradataDictionaryObject;
};
