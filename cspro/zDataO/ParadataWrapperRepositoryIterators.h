#pragma once

#include <zDataO/WrapperRepositoryIterators.h>
#include <zParadataO/Logger.h>
#include <zParadataO/IParadataDriver.h>


class ParadataWrapperRepositoryCaseIterator : public WrapperRepositoryCaseIterator
{
public:
    ParadataWrapperRepositoryCaseIterator(ParadataWrapperRepository& paradata_wrapper_repository, std::shared_ptr<CaseIterator> case_iterator)
        :   WrapperRepositoryCaseIterator(case_iterator),
            m_paradataWrapperRepository(paradata_wrapper_repository)
    {
    }

    bool NextCase(Case& data_case) override
    {
        bool case_read = WrapperRepositoryCaseIterator::NextCase(data_case);

        if( case_read && m_paradataWrapperRepository.m_paradataDriver.GetRecordIteratorLoadCases() )
        {
            m_paradataWrapperRepository.LogEvent(std::make_shared<Paradata::DataRepositoryEvent>(Paradata::DataRepositoryOpenEvent::Action::ReadCase,
                m_paradataWrapperRepository.m_paradataDictionaryObject, data_case.GetUuid(), data_case.GetKey()));
        }

        return case_read;
    }

private:
    ParadataWrapperRepository& m_paradataWrapperRepository;
};
