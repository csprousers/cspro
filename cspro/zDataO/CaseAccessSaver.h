#pragma once

#include <zDataO/DataRepository.h>


///<summary>Helper class to save and restore the current CaseAccess for a repository using RAII.</summary>

class CaseAccessSaver
{
public:
    CaseAccessSaver(DataRepository& repository)
        :   m_repository(repository),
            m_caseAccess(m_repository.GetSharedCaseAccess())
    {
    }

    ~CaseAccessSaver()
    {
        if( m_repository.GetCaseAccess() != m_caseAccess.get() )
            m_repository.ModifyCaseAccess(std::move(m_caseAccess));
    }

private:
    DataRepository& m_repository;
    std::shared_ptr<const CaseAccess> m_caseAccess;
};
