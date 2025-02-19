#pragma once

#include <zDataO/DataRepository.h>


class DataRepositoryTransaction
{
public:
    DataRepositoryTransaction(DataRepository& repository)
        :   m_repository(&repository)
    {
        m_repository->StartTransaction();
    }

    DataRepositoryTransaction(const DataRepositoryTransaction&) = delete;

    DataRepositoryTransaction(DataRepositoryTransaction&& rhs) noexcept
        :   m_repository(rhs.m_repository)
    {
        rhs.m_repository = nullptr;
    }

    ~DataRepositoryTransaction()
    {
        if( m_repository != nullptr )
            m_repository->EndTransaction();
    }

private:
    DataRepository* m_repository;
};
