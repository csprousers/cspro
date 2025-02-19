#pragma once

#include <DataManager/Task.h>


class CaseTask : public Task
{
protected:
    CaseTask();

public:
    void SetCaseProvider(std::shared_ptr<const CaseAccess> case_access, std::shared_ptr<CaseProvider> case_provider);

    // Task overrides
    void Run() override final;

protected:
    size_t GetCasesProcessed() const { return m_casesProcessed; }

    // A subclass must implement ProcessCase.
    virtual void ProcessCase(Case& data_case) = 0;

protected:
    std::shared_ptr<const CaseAccess> m_caseAccess;

private:
    std::shared_ptr<CaseProvider> m_caseProvider;
    size_t m_casesProcessed;
};
