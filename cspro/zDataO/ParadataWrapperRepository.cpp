#include "stdafx.h"
#include "ParadataWrapperRepository.h"
#include "ParadataWrapperRepositoryIterators.h"
#include <zParadataO/Logger.h>
#include <zParadataO/IParadataDriver.h>


void ParadataWrapperRepository::LogEvent(std::shared_ptr<Paradata::Event> event)
{
    m_paradataDriver.RegisterAndLogEvent(event, this);
}


void ParadataWrapperRepository::CreateCaseNotFoundEvent(const CString& key/* = CString()*/)
{
    LogEvent(std::make_shared<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::CaseNotFound,
        m_paradataDictionaryObject, CString(), key));

    throw DataRepositoryException::CaseNotFound();
}


ParadataWrapperRepository::ParadataWrapperRepository(std::shared_ptr<DataRepository> repository, DataRepositoryAccess access_type,
    Paradata::IParadataDriver& paradata_driver, std::shared_ptr<Paradata::NamedObject> paradata_dictionary_object)
    :   WrapperRepository(std::move(repository), access_type),
        m_paradataDriver(paradata_driver),
        m_paradataDictionaryObject(paradata_dictionary_object)
{
}


void ParadataWrapperRepository::Open(DataRepositoryOpenFlag open_flag)
{
    WrapperRepository::Open(open_flag);

    LogEvent(std::make_shared<Paradata::DataRepositoryOpenEvent>(
        m_paradataDictionaryObject,
        m_repository->GetName(DataRepositoryNameType::Full),
        (int)m_repository->GetRepositoryType(),
        (int)m_accessType,
        (int)open_flag));
}


void ParadataWrapperRepository::Close()
{
    WrapperRepository::Close();

    LogEvent(std::make_shared<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::Close, m_paradataDictionaryObject));
}


void ParadataWrapperRepository::ReadCase(Case& data_case, const CString& key)
{
    try
    {
        WrapperRepository::ReadCase(data_case, key);

        LogEvent(std::make_shared<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::ReadCase,
            m_paradataDictionaryObject, data_case.GetUuid(), data_case.GetKey()));
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        CreateCaseNotFoundEvent(key);
    }
}


void ParadataWrapperRepository::ReadCase(Case& data_case, double position_in_repository)
{
    try
    {
        WrapperRepository::ReadCase(data_case, position_in_repository);

        LogEvent(std::make_shared<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::ReadCase,
            m_paradataDictionaryObject, data_case.GetUuid(), data_case.GetKey()));
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        CreateCaseNotFoundEvent();
    }
}


void ParadataWrapperRepository::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/)
{
    WrapperRepository::WriteCase(data_case, write_case_parameter);

    LogEvent(std::make_shared<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryEvent::Action::WriteCase,
        m_paradataDictionaryObject, data_case.GetUuid(), data_case.GetKey(), data_case.IsPartial()));
}


void ParadataWrapperRepository::DeleteCase(double position_in_repository, bool deleted/* = true*/)
{
    CString key;
    CString uuid;
    WrapperRepository::PopulateCaseIdentifiers(key, uuid, position_in_repository);

    WrapperRepository::DeleteCase(position_in_repository, deleted);

    LogEvent(std::make_shared<Paradata::DataRepositoryEvent>(
        deleted ? Paradata::DataRepositoryEvent::Action::DeleteCase : Paradata::DataRepositoryEvent::Action::UndeleteCase,
        m_paradataDictionaryObject, uuid, key));
}


std::unique_ptr<CaseIterator> ParadataWrapperRepository::CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
    std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order, 
    const CaseIteratorParameters* start_parameters/* = nullptr*/, size_t offset/* = 0*/, size_t limit/* = SIZE_MAX*/)
{
    return std::make_unique<ParadataWrapperRepositoryCaseIterator>(*this,
        m_repository->CreateIterator(iteration_content, case_status, iteration_method, iteration_order, start_parameters, offset, limit));
}
