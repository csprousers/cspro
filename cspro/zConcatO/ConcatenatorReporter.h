#pragma once

#include <zUtilO/ConnectionString.h>
#include <zUtilO/ProcessSummary.h>


class ConcatenatorReporter
{
public:
    ConcatenatorReporter(std::shared_ptr<ProcessSummary> process_summary)
        :   m_processSummary(std::move(process_summary))
    {
        ASSERT(m_processSummary != nullptr);
    }

    virtual ~ConcatenatorReporter() { }

    ProcessSummary& GetProcessSummary()
    {
        return *m_processSummary;
    }

    void AddSuccessfullyProcessedTarget(ConnectionString connection_string)
    {
        m_successfullyProcessedTargets.emplace_back(std::move(connection_string));
    }

    const std::vector<ConnectionString>& GetSuccessfullyProcessedTargets() const
    {
        return m_successfullyProcessedTargets;
    }

    virtual bool IsCanceled() const = 0;

    virtual void SetSource(const std::string& source_text) = 0;

    virtual void SetKey(const std::string& key) = 0;

    virtual void ErrorFileOpenFailed(const std::string& file_path) = 0;

    virtual void ErrorDataSourceOpenFailed(const ConnectionString& connection_string, const std::string& error_message) = 0;

    virtual void ErrorInvalidEncoding(const std::string& file_path) = 0;

    virtual void ErrorDuplicateCase(const std::string& key, const ConnectionString& connection_string, const ConnectionString& previous_connection_string) = 0;

    virtual void ErrorOther(const ConnectionString& connection_string, const std::string& error_message) = 0;

private:
    std::shared_ptr<ProcessSummary> m_processSummary;
    std::vector<ConnectionString> m_successfullyProcessedTargets;
};
