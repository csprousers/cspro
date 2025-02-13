#include "stdafx.h"
#include "CacheableCaseWrapperRepository.h"
#include "CacheableCaseWrapperRepositoryCaseIterators.h"
#include <zToolsO/Hash.h>


CacheableCaseWrapperRepository::CacheableCaseWrapperRepository(std::shared_ptr<DataRepository> repository, DataRepositoryAccess access_type)
    :   WrapperRepository(std::move(repository), access_type),
        m_positionsInRepositoryChangeOnModification(!DataRepositoryHelpers::IsTypeSQLiteOrDerived(m_repository->GetRepositoryType()))
{
}


std::shared_ptr<DataRepository> CacheableCaseWrapperRepository::CreateCacheableCaseWrapperRepository(std::shared_ptr<DataRepository> repository, DataRepositoryAccess access_type)
{
    // the CacheableCaseWrapperRepository can only be used in certain circumstances
    if( access_type == DataRepositoryAccess::ReadOnly || access_type == DataRepositoryAccess::ReadWrite )
    {
        return std::shared_ptr<DataRepository>(new CacheableCaseWrapperRepository(std::move(repository), access_type));
    }

    else
    {
        return repository;
    }
}


void CacheableCaseWrapperRepository::ClearCachedIterations()
{
    m_cachedIterations.clear();
}


void CacheableCaseWrapperRepository::ClearCachedCases(bool reuse_cases/* = true*/)
{
    ClearCachedIterations();

    // save any cases to reuse at a future point
    if( reuse_cases )
    {
        for( const auto& kv : m_casesByPosition )
            m_unusedCasesPool.push_back(kv.second);
    }

    else
    {
        m_unusedCasesPool.clear();
    }

    m_casesByKey.clear();
    m_casesByPosition.clear();
}


void CacheableCaseWrapperRepository::ClearCachedCase(const Case& data_case)
{
    ASSERT(!m_positionsInRepositoryChangeOnModification);

    ClearCachedIterations();

    m_casesByKey.erase(data_case.GetKey());

    const auto& case_lookup = m_casesByPosition.find(data_case.GetPositionInRepository());

    // save the case for reuse at a future point
    if( case_lookup != m_casesByPosition.cend() && case_lookup->second.use_count() == 1 )
    {
        m_unusedCasesPool.push_back(case_lookup->second);
        m_casesByPosition.erase(case_lookup);
    }
}


std::shared_ptr<Case> CacheableCaseWrapperRepository::CacheCase(Case& data_case, bool cache_using_key)
{
    std::shared_ptr<Case> cached_data_case;

    if( m_unusedCasesPool.empty() )
    {
        cached_data_case = GetCaseAccess()->CreateCase();
    }

    else
    {
        cached_data_case = m_unusedCasesPool.back();
        m_unusedCasesPool.pop_back();
    }

    *cached_data_case = data_case;

    // not everything is cached by key because a case retrieved by the position in repository
    // may not be accessible by the key (e.g., the second [duplicate] case in the repository with a given key)
    if( cache_using_key )
        m_casesByKey.emplace(cached_data_case->GetKey(), cached_data_case);

    m_casesByPosition.emplace(cached_data_case->GetPositionInRepository(), cached_data_case);

    return cached_data_case;
}


void CacheableCaseWrapperRepository::ReadCase(Case& data_case, const CString& key)
{
    const auto& case_lookup = m_casesByKey.find(key);

    if( case_lookup != m_casesByKey.cend() )
    {
        data_case = *case_lookup->second;
    }

    else
    {
        WrapperRepository::ReadCase(data_case, key);
        CacheCase(data_case, true);
    }
}


void CacheableCaseWrapperRepository::ReadCase(Case& data_case, double position_in_repository)
{
    const auto& case_lookup = m_casesByPosition.find(position_in_repository);

    if( case_lookup != m_casesByPosition.cend() )
    {
        data_case = *case_lookup->second;
    }

    else
    {
        WrapperRepository::ReadCase(data_case, position_in_repository);
        CacheCase(data_case, false);
    }
}


void CacheableCaseWrapperRepository::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/)
{
    // this should only be triggered by writecase calls, which means that we can cache by key as well
    ASSERT(write_case_parameter == nullptr && !data_case.GetDeleted());

    ClearCachedIterations();

    if( m_positionsInRepositoryChangeOnModification )
    {
        ClearCachedCases();
    }

    else
    {
        ClearCachedCase(data_case);
    }

    WrapperRepository::WriteCase(data_case, write_case_parameter);
    CacheCase(data_case, true);
}


void CacheableCaseWrapperRepository::DeleteCase(double position_in_repository, bool deleted/* = true*/)
{
    ClearCachedIterations();

    if( m_positionsInRepositoryChangeOnModification )
    {
        ClearCachedCases();
    }

    // lookup the case so that we can delete it from both the key and position maps
    else if( const auto& case_lookup = m_casesByPosition.find(position_in_repository); case_lookup != m_casesByPosition.cend() )
    {
        ClearCachedCase(*case_lookup->second);
    }

    WrapperRepository::DeleteCase(position_in_repository, deleted);
}


std::unique_ptr<CaseIterator> CacheableCaseWrapperRepository::CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
    std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
    const CaseIteratorParameters* start_parameters/* = nullptr*/, size_t offset/* = 0*/, size_t limit/* = SIZE_MAX*/)
{
    // create a hash value representing the options (except for the iteration content);
    // this method could be smarter and, for example, reuse a past iteration if only the
    // iteration order has changed, but that is rare so it won't be implemented (for now)
    size_t iteration_hash_value = 0;

    Hash::Combine(iteration_hash_value, (int)case_status);

    if( iteration_method.has_value() )
        Hash::Combine(iteration_hash_value, (int)*iteration_method);

    if( iteration_order.has_value() )
        Hash::Combine(iteration_hash_value, (int)*iteration_order);

    if( start_parameters != nullptr )
    {
        Hash::Combine(iteration_hash_value, (int)start_parameters->start_type);

        if( std::holds_alternative<CString>(start_parameters->first_key_or_position) )
        {
            Hash::Combine(iteration_hash_value, wstring_view(std::get<CString>(start_parameters->first_key_or_position)));
        }

        else
        {
            Hash::Combine(iteration_hash_value, std::get<double>(start_parameters->first_key_or_position));
        }

        if( start_parameters->key_prefix.has_value() )
            Hash::Combine(iteration_hash_value, wstring_view(*start_parameters->key_prefix));
    }
    
    Hash::Combine(iteration_hash_value, offset);
    Hash::Combine(iteration_hash_value, limit);

    const auto& cached_iterations_lookup = m_cachedIterations.find(iteration_hash_value);

    // reuse a previous iteration if possible
    if( cached_iterations_lookup != m_cachedIterations.cend() )
    {
        return std::make_unique<CacheableCaseWrapperRepositorySecondPassCaseIterator>(cached_iterations_lookup->second);
    }

    else
    {
        auto iterator = m_repository->CreateIterator(iteration_content, case_status, iteration_method, iteration_order, start_parameters, offset, limit);

        // if not iterating cases, there is no need to wrap the iterator
        if( iteration_content != CaseIterationContent::Case )
        {
            return iterator;
        }

        else
        {
            return std::make_unique<CacheableCaseWrapperRepositoryFirstPassCaseIterator>(this, iteration_hash_value, std::move(iterator));
        }
    }
}
