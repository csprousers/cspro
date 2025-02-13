#include "stdafx.h"
#include "MemoryRepository.h"
#include "MemoryRepositoryIterators.h"
#include <numeric>


MemoryRepository::MemoryRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type)
    :   DataRepository(DataRepositoryType::Memory, std::move(case_access), access_type)
{
}


void MemoryRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    m_caseAccess = std::move(case_access);
}


void MemoryRepository::Open(DataRepositoryOpenFlag /*open_flag*/)
{
    m_cases.clear();
}


void MemoryRepository::Close()
{
    m_cases.clear();
}


void MemoryRepository::DeleteRepository()
{
    Close();
}


const Case& MemoryRepository::GetCase(double position_in_repository) const
{
    size_t cases_index = (size_t)position_in_repository;

    if( cases_index >= m_cases.size() )
        throw DataRepositoryException::CaseNotFound();

    return *m_cases[cases_index];
}


std::optional<size_t> MemoryRepository::GetCaseIndex(const CString& key) const
{
    const auto& case_search = std::find_if(m_cases.cbegin(), m_cases.cend(),
        [&key](const std::shared_ptr<Case>& data_case)
        { return ( data_case->GetKey().Compare(key) == 0 && !data_case->GetDeleted() ); });

    if( case_search != m_cases.cend() )
        return std::distance(m_cases.cbegin(), case_search);

    return std::nullopt;
}


const Case& MemoryRepository::GetCase(const CString& key) const
{
    std::optional<size_t> case_index = GetCaseIndex(key);

    if( !case_index.has_value() )
        throw DataRepositoryException::CaseNotFound();

    return *m_cases[*case_index];    
}


const Case& MemoryRepository::GetCaseByUuid(const CString& uuid) const
{
    const auto& case_search = std::find_if(m_cases.cbegin(), m_cases.cend(),
        [&uuid](const std::shared_ptr<Case>& data_case)
        { return ( data_case->GetUuid().Compare(uuid) == 0 ); });

    if( case_search == m_cases.cend() )
        throw DataRepositoryException::CaseNotFound();

    return *(*case_search);
}


bool MemoryRepository::ContainsCase(const CString& key) const
{
    std::optional<size_t> case_index = GetCaseIndex(key);

    return case_index.has_value();
}


void MemoryRepository::PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const
{
    const Case& data_case = ( !key.IsEmpty() )  ? GetCase(key) :
                            ( !uuid.IsEmpty() ) ? GetCaseByUuid(uuid) :
                                                  GetCase(position_in_repository);

    key = data_case.GetKey();
    uuid = data_case.GetUuid();
    position_in_repository = data_case.GetPositionInRepository();
}


std::optional<CaseKey> MemoryRepository::FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
    const CaseIteratorParameters* start_parameters/* = nullptr*/) const
{
    std::vector<size_t> indices = GetFilteredIndices(CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order, start_parameters);

    if( !indices.empty() )
        return *m_cases[indices.front()];

    return std::nullopt;
}


void MemoryRepository::ReadCase(Case& data_case, const CString& key)
{
    const auto& case_search = std::find_if(m_cases.cbegin(), m_cases.cend(),
        [&key](const std::shared_ptr<Case>& data_case)
        { return ( data_case->GetKey().Compare(key) == 0 && !data_case->GetDeleted() ); });

    if( case_search == m_cases.cend() )
        throw DataRepositoryException::CaseNotFound();

    data_case = *(*case_search);
}


void MemoryRepository::ReadCase(Case& data_case, double position_in_repository)
{
    size_t cases_index = (size_t)position_in_repository;

    if( cases_index >= m_cases.size() )
        throw DataRepositoryException::CaseNotFound();

    data_case = *m_cases[cases_index];
}


void MemoryRepository::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/)
{
    size_t case_insertion_index = m_cases.size();

    if( write_case_parameter != nullptr )
    {
        case_insertion_index = (size_t)write_case_parameter->GetPositionInRepository();

        if( write_case_parameter->IsInsertParameter() )
        {
            // adjust the repository position for any existing cases
            std::for_each(m_cases.begin() + case_insertion_index, m_cases.end(),
                [](std::shared_ptr<Case>& shift_data_case)
                { shift_data_case->SetPositionInRepository(shift_data_case->GetPositionInRepository() + 1); });

            // and add the new case
            m_cases.insert(m_cases.begin() + case_insertion_index, m_caseAccess->CreateCase(true));
        }
    }

    else if( m_accessType == DataRepositoryAccess::ReadWrite )
    {
        // see if an exising case has to be modified
        const auto& case_modification_point = std::find_if(m_cases.cbegin(), m_cases.cend(),
            [&data_case](const std::shared_ptr<Case>& search_data_case)
            { return ( search_data_case->GetKey().Compare(data_case.GetKey()) == 0 ); });

        // if the case already exists, use its UUID
        if( case_modification_point != m_cases.cend() )
        {
            case_insertion_index = case_modification_point - m_cases.cbegin();
            data_case.SetUuid(CS2WS((*case_modification_point)->GetUuid()));
        }

        // otherwise assign a new UUID
        else
        {
            data_case.SetUuid(CreateUuid());
        }
    }

    // if adding to the end, add a slot for it
    if( case_insertion_index == m_cases.size() )
        m_cases.emplace_back(m_caseAccess->CreateCase(true));

    Case& new_case = *m_cases[case_insertion_index];
    new_case = data_case;

    // set a UUID if one doesn't exist and set the new repository position
    new_case.GetOrCreateUuid();
    new_case.SetPositionInRepository(case_insertion_index);
}


void MemoryRepository::DeleteCase(double position_in_repository, bool deleted/* = true*/)
{
    Case& data_case = GetCase(position_in_repository);
    data_case.SetDeleted(deleted);
}


std::vector<size_t> MemoryRepository::GetFilteredIndices(CaseIterationCaseStatus case_status,
    std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
    const CaseIteratorParameters* start_parameters) const
{
    std::vector<size_t> indices(m_cases.size());
    std::iota(indices.begin(), indices.end(), 0);

    // process any filters
    if( start_parameters != nullptr )
    {
        bool use_key_prefix = false;
        bool use_operators = false;

        if( start_parameters->key_prefix.has_value() && !start_parameters->key_prefix->IsEmpty() )
        {
            use_key_prefix = true;

            use_operators = std::holds_alternative<CString>(start_parameters->first_key_or_position) ?
                !std::get<CString>(start_parameters->first_key_or_position).IsEmpty() :
                ( std::get<double>(start_parameters->first_key_or_position) != -1 );
        }

        else
        {
            use_operators = true;
        }

        FilterIndices(indices, [&](const Case& data_case)
        {
            if( use_key_prefix && data_case.GetKey().Find(*start_parameters->key_prefix) != 0 )
            {
                return false;
            }

            else if( use_operators )
            {
                double comparison;

                if( std::holds_alternative<CString>(start_parameters->first_key_or_position) )
                {
                    comparison = data_case.GetKey().Compare(std::get<CString>(start_parameters->first_key_or_position));
                }

                else
                {
                    comparison = data_case.GetPositionInRepository() - std::get<double>(start_parameters->first_key_or_position);
                }

                return   ( start_parameters->start_type == CaseIterationStartType::LessThan )          ?   ( comparison < 0 ) :
                         ( start_parameters->start_type == CaseIterationStartType::LessThanEquals )    ?   ( comparison <= 0 ) :
                         ( start_parameters->start_type == CaseIterationStartType::GreaterThanEquals ) ?   ( comparison >= 0 ) :
                       /*( start_parameters->start_type == CaseIterationStartType::GreaterThan )       ? */( comparison > 0 );
            }

            return true;
        });
    }


    // filter on case properties
    if( case_status != CaseIterationCaseStatus::All )
        FilterIndices(indices, [](const Case& data_case) { return !data_case.GetDeleted(); });

    if( case_status == CaseIterationCaseStatus::PartialsOnly )
    {
        FilterIndices(indices, [](const Case& data_case) { return data_case.IsPartial(); });
    }

    else if( case_status == CaseIterationCaseStatus::DuplicatesOnly )
    {
        FilterIndices(indices, [&](const Case& data_case)
        {
            const auto& duplicate_search = std::find_if(m_cases.cbegin(), m_cases.cend(), [&](const std::shared_ptr<Case>& search_data_case)
            {
                return ( search_data_case.get() != &data_case && search_data_case->GetKey().Compare(data_case.GetKey()) == 0 );
            });

            return ( duplicate_search != m_cases.cend() );
        });
    }


    // potentially sort in key order / descending order
    if( iteration_method == CaseIterationMethod::KeyOrder )
    {
        std::sort(indices.begin(), indices.end(), [this](size_t index1, size_t index2)
            { return ( m_cases[index1]->GetKey().Compare(m_cases[index2]->GetKey()) < 0 ); });
    }

    if( iteration_order == CaseIterationOrder::Descending )
        std::reverse(indices.begin(), indices.end());


    return indices;
}


void MemoryRepository::FilterIndices(std::vector<size_t>& indices, const std::function<bool(const Case&)>& filter_function) const
{
    // initially mark any filtered-out indices as SIZE_MAX
    for( size_t& index : indices )
    {
        if( index != SIZE_MAX && !filter_function(*m_cases[index]) )
            index = SIZE_MAX;
    }

    // and then remove them
    indices.erase(std::remove(indices.begin(), indices.end(), SIZE_MAX), indices.end());
}


size_t MemoryRepository::GetNumberCases() const
{
    return GetNumberCases(CaseIterationCaseStatus::NotDeletedOnly);
}


size_t MemoryRepository::GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters/* = nullptr*/) const
{
    // calculate non-duplicate and non-filter queries as they are easy
    if(  start_parameters == nullptr )
    {
        if( case_status == CaseIterationCaseStatus::All )
        {
            return m_cases.size();
        }

        else if( case_status == CaseIterationCaseStatus::NotDeletedOnly )
        {
            return std::count_if(m_cases.cbegin(), m_cases.cend(), [](const std::shared_ptr<Case>& data_case)
            {
                return !data_case->GetDeleted();
            });
        }

        else if( case_status == CaseIterationCaseStatus::PartialsOnly )
        {
            return std::count_if(m_cases.cbegin(), m_cases.cend(), [](const std::shared_ptr<Case>& data_case)
            {
                return !data_case->GetDeleted() && data_case->IsPartial();
            });
        }
    }

    // otherwise, get the value in a non-optimized way
    std::vector<size_t> indices = GetFilteredIndices(case_status, std::nullopt, std::nullopt, start_parameters);

    return indices.size();
}


std::unique_ptr<CaseIterator> MemoryRepository::CreateIterator(CaseIterationContent /*iteration_content*/, CaseIterationCaseStatus case_status,
    std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order, 
    const CaseIteratorParameters* start_parameters/* = nullptr*/, size_t offset/* = 0*/, size_t limit/* = SIZE_MAX*/)
{
    std::vector<size_t> indices = GetFilteredIndices(case_status, iteration_method, iteration_order, start_parameters);

    // process the offset and limit
    if( offset != 0 || limit != SIZE_MAX )
    {
        offset = std::min(indices.size(), offset);

        if( limit != SIZE_MAX )
            limit = offset + limit;
        
        limit = std::min(indices.size(), limit);

        indices = std::vector<size_t>(indices.cbegin() + offset, indices.cbegin() + limit);
    }

    return std::make_unique<MemoryRepositoryCaseIterator>(m_cases, std::move(indices));
}
