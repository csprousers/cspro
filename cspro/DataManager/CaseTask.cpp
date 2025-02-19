#include "StdAfx.h"
#include "CaseTask.h"
#include "TaskRunner.h"


CaseTask::CaseTask()
    :   m_casesProcessed(0)
{
}


void CaseTask::SetCaseProvider(std::shared_ptr<const CaseAccess> case_access, std::shared_ptr<CaseProvider> case_provider)
{
    m_caseAccess = std::move(case_access);
    m_caseProvider = std::move(case_provider);
}


void CaseTask::Run()
{
    ASSERT(m_taskRunner != nullptr && m_cancelFlag != nullptr);
    ASSERT(m_caseAccess != nullptr && m_caseProvider != nullptr);

    const std::unique_ptr<Case> data_case = m_caseAccess->CreateCase();
    data_case->SetCaseConstructionReporter(m_taskRunner->GetCaseConstructionReporter(*m_caseAccess));

    const size_t number_cases = m_caseProvider->GetNumberCases();

    m_taskRunner->LogText("Processing %d case%s from the dictionary: %s\n",
                          static_cast<int>(number_cases), PluralizeWord(number_cases),
                          m_caseAccess->GetDataDict().GetName().c_str());

    // the progress bar will be updated every 1% and the case details every 5%
    constexpr double UpdatePercentPercentInterval = 1;
    constexpr double UpdateCaseDetailsPercentInterval = 5;
    const double percent_multiplier = CreatePercentMultiplier(number_cases);
    double percent = 0;
    double next_update_percent_percent = UpdatePercentPercentInterval;
    double next_update_case_details_percent = UpdateCaseDetailsPercentInterval;

    // the case details will be also be updated every 3 seconds so that updates
    // will be shown even when the progress bar is not frequently updated
    constexpr std::chrono::nanoseconds DisplayCaseDetailsSecondsInterval(3000000000);
    std::optional<std::chrono::steady_clock::time_point> last_updated_time_point;
    double position_in_repository_of_last_updated_case_details = -1;

    auto update_case_details = [&]()
    {
        m_taskRunner->LogText("Last processed case (%d / %d): %s", static_cast<int>(m_casesProcessed),
                                                                   static_cast<int>(number_cases),
                                                                   data_case->GetSingleLineKey().c_str());

        position_in_repository_of_last_updated_case_details = data_case->GetPositionInRepository();
    };

    while( m_caseProvider->NextCase(*data_case) )
    {
        ProcessCase(*data_case);

        ++m_casesProcessed;
        percent += percent_multiplier;

        // check for cancelation
        if( IsCanceled() )
            throw CanceledException();

        // periodically update the progress bar and the case details
        if( percent >= next_update_percent_percent )
        {
            m_taskRunner->UpdateProgress(static_cast<int>(percent));
            next_update_percent_percent = percent + UpdatePercentPercentInterval;

            static_assert(UpdateCaseDetailsPercentInterval > UpdatePercentPercentInterval);

            if( percent >= next_update_case_details_percent )
            {
                update_case_details();
                next_update_case_details_percent = percent + UpdateCaseDetailsPercentInterval;
                last_updated_time_point.reset();
            }
        }

        else
        {
            const std::chrono::steady_clock::time_point current_time_point = std::chrono::steady_clock::now();

            if( !last_updated_time_point.has_value() )
            {
                last_updated_time_point = current_time_point;
            }

            else if( ( current_time_point - *last_updated_time_point ) >= DisplayCaseDetailsSecondsInterval )
            {
                update_case_details();
                last_updated_time_point = current_time_point;
            }
        }
    }

    // always show the last updated case
    if( position_in_repository_of_last_updated_case_details != data_case->GetPositionInRepository() )
        update_case_details();

    m_taskRunner->LogText();
}
