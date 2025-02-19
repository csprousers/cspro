#pragma once

#include <zDataO/WrapperRepositoryIterators.h>


class ParadataWrapperRepositoryCaseIterator : public WrapperRepositoryCaseIterator
{
public:
    ParadataWrapperRepositoryCaseIterator(ParadataWrapperRepository& paradata_wrapper_repository, std::unique_ptr<CaseIterator> case_iterator);

    bool NextCase(Case& data_case) override;

private:
    ParadataWrapperRepository& m_paradataWrapperRepository;
};
