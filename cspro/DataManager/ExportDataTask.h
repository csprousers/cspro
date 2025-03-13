#pragma once

#include <DataManager/CaseTask.h>


class ExportDataTask : public CaseTask
{
public:
    ExportDataTask(std::vector<ConnectionString> export_connection_strings);

protected:
    // Task and CaseTask overrides
    void Initialize() override;
    void ProcessCase(Case& data_case) override;
    void Finalize(Result result) override;

private:
    std::vector<ConnectionString> m_exportConnectionStrings;
    std::vector<std::unique_ptr<DataRepository>> m_exportDataRepositories;
};
