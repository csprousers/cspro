#include "StdAfx.h"
#include "CaseProvider.h"
#include <zDataO/CaseIterator.h>


// --------------------------------------------------------------------------
// SingleCaseProvider
// --------------------------------------------------------------------------

SingleCaseProvider::SingleCaseProvider(std::shared_ptr<const Case> data_case)
    :   m_case(std::move(data_case)),
        m_processedCase(false)
{
    ASSERT(m_case != nullptr);
}


const Case* SingleCaseProvider::GetCaseIfSingleCaseOperation() noexcept
{
    return m_case.get();
}


size_t SingleCaseProvider::GetNumberCases()
{
    return 1;
}


bool SingleCaseProvider::NextCase(Case& data_case)
{
    if( !m_processedCase )
    {
        m_processedCase = true;
        data_case = *m_case;
        return true;
    }

    return false;
}



// --------------------------------------------------------------------------
// DataRepositoryCaseProvider
// --------------------------------------------------------------------------

DataRepositoryCaseProvider::DataRepositoryCaseProvider(std::shared_ptr<DataRepository> data_repository,
                                                       std::shared_ptr<const CaseIteratorSettings> case_iterator_settings/* = nullptr*/)
    :   m_dataRepository(std::move(data_repository)),
        m_caseIteratorSettings(std::move(case_iterator_settings))
{
    ASSERT(m_dataRepository != nullptr);

    if( m_caseIteratorSettings == nullptr )
    {
        auto settings = std::make_unique<CaseIteratorSettings>();

        settings->SetMethod(CaseIterationMethod::SequentialOrder);
        settings->SetOrder(CaseIterationOrder::Ascending);

        m_caseIteratorSettings = std::move(settings);
    }
}


size_t DataRepositoryCaseProvider::GetNumberCases()
{
    if( !m_numberCases.has_value() )
    {
        m_numberCases = m_dataRepository->GetNumberCases(m_caseIteratorSettings->GetStatus(),
                                                         m_caseIteratorSettings->GetParameters());
    }

    return *m_numberCases;
}


bool DataRepositoryCaseProvider::NextCase(Case& data_case)
{
    if( m_caseIterator == nullptr )
        m_caseIterator = m_dataRepository->CreateIterator(CaseIterationContent::Case, *m_caseIteratorSettings);

    return m_caseIterator->NextCase(data_case);
}



// --------------------------------------------------------------------------
// SelectiveDataRepositoryCaseProvider
// --------------------------------------------------------------------------

SelectiveDataRepositoryCaseProvider::SelectiveDataRepositoryCaseProvider(std::shared_ptr<DataRepository> data_repository,
                                                                         const std::vector<std::shared_ptr<const CaseSummary>>& selected_case_summaries)
    :   m_dataRepository(std::move(data_repository)),
        m_caseCounter(0)
{
    m_positionsInRepository.reserve(selected_case_summaries.size());

    for( const std::shared_ptr<const CaseSummary>& case_summary : selected_case_summaries )
        m_positionsInRepository.emplace_back(case_summary->GetPositionInRepository());
}


size_t SelectiveDataRepositoryCaseProvider::GetNumberCases()
{
    return m_positionsInRepository.size();
}


bool SelectiveDataRepositoryCaseProvider::NextCase(Case& data_case)
{
    if( m_caseCounter == m_positionsInRepository.size() )
        return false;

    m_dataRepository->ReadCase(data_case, m_positionsInRepository[m_caseCounter]);
    ++m_caseCounter;

    return true;
}
