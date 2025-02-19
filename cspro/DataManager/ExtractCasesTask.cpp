#include "StdAfx.h"
#include "ExtractCasesTask.h"
#include "TaskRunner.h"


ExtractCasesTask::ExtractCasesTask(ConnectionString output_connection_string)
    :   m_outputConnectionString(std::move(output_connection_string))
{
}


void ExtractCasesTask::Initialize()
{
    m_taskRunner->SetTitle("Extracting cases...");

    m_outputDataRepository = DataRepository::CreateAndOpen(m_caseAccess,
                                                           m_outputConnectionString,
                                                           DataRepositoryAccess::BatchOutput,
                                                           DataRepositoryOpenFlag::CreateNew);

    m_taskRunner->LogText("Saving cases to data source: " + m_outputDataRepository->GetName(DataRepositoryNameType::Full));
    m_taskRunner->LogText();
}


void ExtractCasesTask::ProcessCase(Case& data_case)
{
    m_outputDataRepository->WriteCase(data_case);
}


void ExtractCasesTask::Finalize(const Result result)
{
    if( result == Result::Complete )
    {
        const size_t cases_processed = GetCasesProcessed();

        m_taskRunner->LogText("Process summary:");
        m_taskRunner->LogText("    Cases processed: %d", static_cast<int>(cases_processed));

        m_taskRunner->LogText();
        m_taskRunner->LogText("Successfully extracted %d case%s.", static_cast<int>(cases_processed),
                                                                   PluralizeWord(cases_processed));
    }

    else if( m_outputDataRepository != nullptr )
    {
        m_taskRunner->LogText("Deleting the data source: " + m_outputDataRepository->GetName(DataRepositoryNameType::Full));
        m_outputDataRepository->DeleteRepository();
    }

    m_outputDataRepository.reset();
}
