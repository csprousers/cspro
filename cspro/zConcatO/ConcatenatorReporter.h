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
    
    void AddSuccessfullyProcessedFile(ConnectionString connection_string)
    {
        m_successfullyProcessedFiles.emplace_back(std::move(connection_string));
    }

    const std::vector<ConnectionString>& GetSuccessfullyProcessedFiles() const
    {
        return m_successfullyProcessedFiles;
    }    

    virtual bool IsCanceled() const = 0;

    virtual void SetSource(NullTerminatedString source) = 0;

    virtual void SetKey(NullTerminatedString key) = 0;

    virtual void ErrorFileOpenFailed(const ConnectionString& connection_string) = 0;

    virtual void ErrorInvalidEncoding(const ConnectionString& connection_string) = 0;

    virtual void ErrorDuplicateCase(NullTerminatedString key, const ConnectionString& connection_string, const ConnectionString& previous_connection_string) = 0;

    virtual void ErrorOther(const ConnectionString& connection_string, NullTerminatedString error_message) = 0;

private:
    std::shared_ptr<ProcessSummary> m_processSummary;
    std::vector<ConnectionString> m_successfullyProcessedFiles;
};
