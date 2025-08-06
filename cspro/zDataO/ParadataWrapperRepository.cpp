#include "stdafx.h"
#include "ParadataWrapperRepository.h"
#include "ParadataWrapperRepositoryIterators.h"
#include <zParadataO/Logger.h>
#include <zParadataO/ParadataDriver.h>


// --------------------------------------------------------------------------
// ParadataWrapperRepository
// --------------------------------------------------------------------------

ParadataWrapperRepository::ParadataWrapperRepository(cs::non_null_shared_or_raw_ptr<DataRepository> repository,
                                                     cs::non_null_shared_or_raw_ptr<Paradata::ParadataDriver> paradata_driver,
                                                     std::shared_ptr<Paradata::NamedObject> paradata_dictionary_object)
    :   WrapperRepository(std::move(repository)),
        m_paradataDriver(std::move(paradata_driver)),
        m_paradataDictionaryObject(std::move(paradata_dictionary_object))
{
}


template<typename EventT, typename... Args>
void ParadataWrapperRepository::LogEvent(Args&&... args)
{
    m_paradataDriver->RegisterAndLogEvent(std::make_unique<EventT>(std::forward<Args>(args)...), this);
}


void ParadataWrapperRepository::Open(const DataRepositoryOpenFlag open_flag)
{
    WrapperRepository::Open(open_flag);

    LogEvent<Paradata::DataRepositoryOpenEvent>(m_paradataDictionaryObject,
                                                m_repository->GetName(DataRepositoryNameType::Full),
                                                m_repository->GetRepositoryType(), m_accessType, open_flag);
}


void ParadataWrapperRepository::Close()
{
    WrapperRepository::Close();

    LogEvent<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::Close,
                                            m_paradataDictionaryObject);
}


template<bool read_parameter_is_uuid, typename T>
void ParadataWrapperRepository::ReadCaseWorker(Case& data_case, const T& read_parameter)
{
    static_assert(!read_parameter_is_uuid || std::is_same_v<T, std::string>);

    try
    {
        if constexpr(read_parameter_is_uuid)
        {
            WrapperRepository::ReadCaseByUuid(data_case, read_parameter);
        }

        else
        {
            WrapperRepository::ReadCase(data_case, read_parameter);
        }

        LogEvent<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::ReadCase,
                                                m_paradataDictionaryObject, data_case.GetUuid(), data_case.GetKey());
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        const std::string* case_uuid = &SO::Empty_string;
        const std::string* case_key = &SO::Empty_string;

        if constexpr(std::is_same_v<T, std::string>)
            ( read_parameter_is_uuid ? case_uuid : case_key ) = &read_parameter;

        LogEvent<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::CaseNotFound,
                                                m_paradataDictionaryObject,
                                                *case_uuid,
                                                *case_key);

        throw DataRepositoryException::CaseNotFound();
    }
}


void ParadataWrapperRepository::ReadCase(Case& data_case, const std::string& key)
{
    ReadCaseWorker<false>(data_case, key);
}


void ParadataWrapperRepository::ReadCase(Case& data_case, const double position_in_repository)
{
    ReadCaseWorker<false>(data_case, position_in_repository);
}


void ParadataWrapperRepository::ReadCaseByUuid(Case& data_case, const std::string& uuid)
{
    ReadCaseWorker<true>(data_case, uuid);
}


void ParadataWrapperRepository::WriteCase(Case& data_case, WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    WrapperRepository::WriteCase(data_case, write_case_parameter);

    LogEvent<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::WriteCase,
                                            m_paradataDictionaryObject, data_case.GetUuid(), data_case.GetKey(), data_case.IsPartial());
}


template<typename... Args>
void ParadataWrapperRepository::DeleteCaseWorker(std::string key, double position_in_repository, bool deleted, Args const&... delete_arguments)
{
    std::string uuid;
    WrapperRepository::PopulateCaseIdentifiers(key, uuid, position_in_repository);

    WrapperRepository::DeleteCase(delete_arguments...);

    LogEvent<Paradata::DataRepositoryEvent>(deleted ? Paradata::DataRepositoryEvent::Action::DeleteCase : Paradata::DataRepositoryEvent::Action::UndeleteCase,
                                            m_paradataDictionaryObject, std::move(uuid), std::move(key));
}


void ParadataWrapperRepository::DeleteCase(const double position_in_repository, const bool deleted/* = true*/)
{
    DeleteCaseWorker(std::string(), position_in_repository, deleted, position_in_repository, deleted);
}


void ParadataWrapperRepository::DeleteCase(const std::string& key)
{
    DeleteCaseWorker(key, 0, true, key);
}


std::unique_ptr<CaseIterator> ParadataWrapperRepository::CreateIterator(const CaseIterationContent iteration_content,
                                                                        const CaseIteratorSettings& iterator_settings,
                                                                        const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    std::unique_ptr<CaseIterator> case_iterator = WrapperRepository::CreateIterator(iteration_content, iterator_settings,
                                                                                    offset, limit);

    return std::make_unique<ParadataWrapperRepositoryCaseIterator>(*this, std::move(case_iterator));
}



// --------------------------------------------------------------------------
// ParadataWrapperRepositoryCaseIterator
// --------------------------------------------------------------------------

ParadataWrapperRepositoryCaseIterator::ParadataWrapperRepositoryCaseIterator(ParadataWrapperRepository& paradata_wrapper_repository,
                                                                             std::unique_ptr<CaseIterator> case_iterator)
    :   WrapperRepositoryCaseIterator(std::move(case_iterator)),
        m_paradataWrapperRepository(paradata_wrapper_repository)
{
}


bool ParadataWrapperRepositoryCaseIterator::NextCase(Case& data_case)
{
    const bool case_read = WrapperRepositoryCaseIterator::NextCase(data_case);

    if( case_read && m_paradataWrapperRepository.m_paradataDriver->GetRecordIteratorLoadCases() )
    {
        m_paradataWrapperRepository.LogEvent<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryOpenEvent::Action::ReadCase,
                                                                            m_paradataWrapperRepository.m_paradataDictionaryObject,
                                                                            data_case.GetUuid(),
                                                                            data_case.GetKey());
    }

    return case_read;
}
