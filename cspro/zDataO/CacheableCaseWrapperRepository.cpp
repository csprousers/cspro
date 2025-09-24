#include "stdafx.h"
#include "CacheableCaseWrapperRepository.h"
#include "CacheableCaseWrapperRepositoryCaseIterators.h"
#include <zToolsO/Hash.h>


// --------------------------------------------------------------------------
// CacheableCaseWrapperRepository
// --------------------------------------------------------------------------

CacheableCaseWrapperRepository::CacheableCaseWrapperRepository(std::shared_ptr<DataRepository> repository)
    :   WrapperRepository(std::move(repository)),
        m_positionsInRepositoryChangeOnModification(!DataRepositoryHelpers::IsTypeSQLiteOrDerived(m_repository->GetRepositoryType()))
{
}


std::shared_ptr<DataRepository> CacheableCaseWrapperRepository::CreateCacheableCaseWrapperRepository(std::shared_ptr<DataRepository> repository)
{
    ASSERT(repository != nullptr);

    // the CacheableCaseWrapperRepository can only be used in certain circumstances
    if( repository->GetRepositoryAccess() == DataRepositoryAccess::ReadOnly ||
        repository->GetRepositoryAccess() == DataRepositoryAccess::ReadWrite )
    {
        return std::shared_ptr<DataRepository>(new CacheableCaseWrapperRepository(std::move(repository)));
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


void CacheableCaseWrapperRepository::ClearCachedCases(const bool reuse_cases)
{
    ClearCachedIterations();

    // save any cases to reuse at a future point
    if( reuse_cases )
    {
        for( const auto& [position, data_case] : m_casesByPosition )
            m_unusedCasesPool.emplace_back(data_case);
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
        m_unusedCasesPool.emplace_back(case_lookup->second);
        m_casesByPosition.erase(case_lookup);
    }
}


std::shared_ptr<Case> CacheableCaseWrapperRepository::CacheCase(Case& data_case, const bool cache_using_key)
{
    std::shared_ptr<Case> cached_data_case;

    if( m_unusedCasesPool.empty() )
    {
        cached_data_case = GetCaseAccess().CreateCase();
    }

    else
    {
        cached_data_case = std::move(m_unusedCasesPool.back());
        m_unusedCasesPool.pop_back();
    }

    *cached_data_case = data_case;

    // not everything is cached by key because a case retrieved by the position in repository or UUID
    // may not be accessible by the key (e.g., the second [duplicate] case in the repository with a given key)
    if( cache_using_key )
        m_casesByKey.emplace(cached_data_case->GetKey(), cached_data_case);

    m_casesByPosition.emplace(cached_data_case->GetPositionInRepository(), cached_data_case);

    return cached_data_case;
}


ISyncableDataRepository* CacheableCaseWrapperRepository::GetSyncableDataRepository()
{
    ClearCachedCases(true);
    return WrapperRepository::GetSyncableDataRepository();
}


void CacheableCaseWrapperRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    ClearCachedCases(false);
    WrapperRepository::ModifyCaseAccess(std::move(case_access));
}


void CacheableCaseWrapperRepository::ReadCase(Case& data_case, const std::string& key)
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


void CacheableCaseWrapperRepository::ReadCase(Case& data_case, const double position_in_repository)
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


void CacheableCaseWrapperRepository::ReadCaseByUuid(Case& data_case, const std::string& uuid)
{
    WrapperRepository::ReadCaseByUuid(data_case, uuid);
    CacheCase(data_case, false);
}


void CacheableCaseWrapperRepository::WriteCase(Case& data_case, WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    // this should only be triggered by writecase calls, which means that we can cache by key as well
    ASSERT(write_case_parameter == nullptr && !data_case.GetDeleted());

    ClearCachedIterations();

    if( m_positionsInRepositoryChangeOnModification )
    {
        ClearCachedCases(true);
    }

    else
    {
        ClearCachedCase(data_case);
    }

    WrapperRepository::WriteCase(data_case, write_case_parameter);
    CacheCase(data_case, true);
}


template<typename MapT, typename LookupT>
void CacheableCaseWrapperRepository::DeleteCaseWorker(MapT& cases_map, const LookupT& lookup_value)
{
    ClearCachedIterations();

    if( m_positionsInRepositoryChangeOnModification )
    {
        ClearCachedCases(true);
    }

    // lookup the case so that we can delete it from both the key and position maps
    else
    {
        const auto& case_lookup = cases_map.find(lookup_value);

        if( case_lookup != cases_map.cend() )
            ClearCachedCase(*case_lookup->second);
    }
}


void CacheableCaseWrapperRepository::DeleteCase(const double position_in_repository, const bool deleted/* = true*/)
{
    DeleteCaseWorker(m_casesByPosition, position_in_repository);

    WrapperRepository::DeleteCase(position_in_repository, deleted);
}


void CacheableCaseWrapperRepository::DeleteCase(const std::string& key)
{
    DeleteCaseWorker(m_casesByKey, key);

    WrapperRepository::DeleteCase(key);
}


std::unique_ptr<CaseIterator> CacheableCaseWrapperRepository::CreateIterator(const CaseIterationContent iteration_content,
                                                                             const CaseIteratorSettings& iterator_settings,
                                                                             const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    // create a hash value representing the options (except for the iteration content);
    // this method could be smarter and, for example, reuse a past iteration if only the
    // iteration order has changed, but that is rare so it won't be implemented (for now)
    size_t iteration_hash_value = 0;

    Hash::Combine(iteration_hash_value, static_cast<int>(iterator_settings.GetStatus()));

    if( iterator_settings.GetMethod().has_value() )
        Hash::Combine(iteration_hash_value, static_cast<int>(*iterator_settings.GetMethod()));

    if( iterator_settings.GetOrder().has_value() )
        Hash::Combine(iteration_hash_value, static_cast<int>(*iterator_settings.GetOrder()));

    const CaseIteratorParameters* const start_parameters = iterator_settings.GetParameters();

    if( start_parameters != nullptr )
    {
        Hash::Combine(iteration_hash_value, static_cast<int>(start_parameters->start_type));

        if( std::holds_alternative<std::string>(start_parameters->first_key_or_position) )
        {
            Hash::Combine(iteration_hash_value, std::get<std::string>(start_parameters->first_key_or_position));
        }

        else
        {
            Hash::Combine(iteration_hash_value, std::get<double>(start_parameters->first_key_or_position));
        }

        if( start_parameters->key_prefix.has_value() )
            Hash::Combine(iteration_hash_value, *start_parameters->key_prefix);
    }

    Hash::Combine(iteration_hash_value, offset);
    Hash::Combine(iteration_hash_value, limit);

    const auto& cached_iterations_lookup = m_cachedIterations.find(iteration_hash_value);

    // reuse a previous iteration if possible
    if( cached_iterations_lookup != m_cachedIterations.cend() )
    {
        return std::make_unique<CCWR_SecondPassCaseIterator>(cached_iterations_lookup->second);
    }

    else
    {
        std::unique_ptr<CaseIterator> case_iterator = WrapperRepository::CreateIterator(iteration_content, iterator_settings,
                                                                                        offset, limit);

        // if not iterating cases, there is no need to wrap the iterator
        if( iteration_content != CaseIterationContent::Case )
        {
            return case_iterator;
        }

        else
        {
            return std::make_unique<CCWR_FirstPassCaseIterator>(*this, iteration_hash_value, std::move(case_iterator));
        }
    }
}



// --------------------------------------------------------------------------
// CCWR_FirstPassCaseIterator
// --------------------------------------------------------------------------

CCWR_FirstPassCaseIterator::CCWR_FirstPassCaseIterator(CacheableCaseWrapperRepository& cacheable_case_wrapper_repository,
                                                       const size_t iteration_hash_value, std::unique_ptr<CaseIterator> case_iterator)
    :   WrapperRepositoryCaseIterator(std::move(case_iterator)),
        m_cacheableCaseWrapperRepository(cacheable_case_wrapper_repository),
        m_iterationHashValue(iteration_hash_value)
{
}


bool CCWR_FirstPassCaseIterator::NextCase(Case& data_case)
{
    const bool case_read = WrapperRepositoryCaseIterator::NextCase(data_case);

    if( case_read )
    {
        m_cachedCases.emplace_back(m_cacheableCaseWrapperRepository.CacheCase(data_case, false));
    }

    // when all cases have been read, the iteration order can be saved for future use
    else
    {
        m_cacheableCaseWrapperRepository.m_cachedIterations.emplace(m_iterationHashValue, m_cachedCases);
    }

    return case_read;
}



// --------------------------------------------------------------------------
// CCWR_SecondPassCaseIterator
// --------------------------------------------------------------------------

CCWR_SecondPassCaseIterator::CCWR_SecondPassCaseIterator(const std::vector<std::shared_ptr<Case>>& cases)
    :   m_cases(cases),
        m_iterator(m_cases.cbegin()),
        m_percentMultiplier(CreatePercentMultiplier(m_cases.size()))
{
}


bool CCWR_SecondPassCaseIterator::NextCaseKey(CaseKey& case_key)
{
    return Next(case_key);
}


bool CCWR_SecondPassCaseIterator::NextCaseSummary(CaseSummary& case_summary)
{
    return Next(case_summary);
}


bool CCWR_SecondPassCaseIterator::NextCase(Case& data_case)
{
    return Next(data_case);
}


int CCWR_SecondPassCaseIterator::GetPercentRead() const
{
    const size_t cases_read = m_iterator - m_cases.cbegin();
    return static_cast<int>(cases_read * m_percentMultiplier);
}


template<typename T>
bool CCWR_SecondPassCaseIterator::CCWR_SecondPassCaseIterator::Next(T& case_object)
{
    if( m_iterator != m_cases.cend() )
    {
        case_object = *(*m_iterator);
        ++m_iterator;
        return true;
    }

    return false;
}
