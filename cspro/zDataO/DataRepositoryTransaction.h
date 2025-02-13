#pragma once

#include <zDataO/DataRepository.h>

class DataRepositoryTransaction
{
public:
    DataRepositoryTransaction(DataRepository& repository)
        :   m_repository(repository)
    {
        m_repository.StartTransaction();
    }

    ~DataRepositoryTransaction()
    {
        m_repository.EndTransaction();
    }

private:
    DataRepository& m_repository;
};
