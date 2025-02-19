#pragma once

#include <DataManager/CaseTask.h>


class ExtractCasesTask : public CaseTask
{
public:
    ExtractCasesTask(ConnectionString output_connection_string);

protected:
    // Task and CaseTask overrides
    void Initialize() override;
    void ProcessCase(Case& data_case) override;
    void Finalize(Result result) override;

private:
    ConnectionString m_outputConnectionString;
    std::unique_ptr<DataRepository> m_outputDataRepository;
};
