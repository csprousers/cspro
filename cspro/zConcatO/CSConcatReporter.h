#pragma once

#include <zConcatO/ConcatenatorReporter.h>
#include <zCaseO/StringBasedCaseConstructionReporter.h>

class ProcessSummaryDlg;


class CSConcatReporter : public ConcatenatorReporter, public StringBasedCaseConstructionReporter
{
public:
    CSConcatReporter(ProcessSummaryDlg& process_summary_dlg, std::shared_ptr<ProcessSummary> process_summary);

    // the tuple is the case key, if defined, and then the error
    const std::vector<std::tuple<std::unique_ptr<std::wstring>, std::wstring>>& GetErrors() const { return m_errors; }

    bool IsCanceled() const override;

    void SetSource(NullTerminatedString source) override;
    void SetKey(NullTerminatedString key) override;

    void ErrorFileOpenFailed(const ConnectionString& connection_string) override;    
    void ErrorInvalidEncoding(const ConnectionString& connection_string) override;    
    void ErrorDuplicateCase(NullTerminatedString key, const ConnectionString& connection_string, const ConnectionString& previous_connection_string) override;
    void ErrorOther(const ConnectionString& connection_string, NullTerminatedString error_message) override;

protected:
    void WriteString(NullTerminatedString key, NullTerminatedString message) override;

private:
    ProcessSummaryDlg& m_processSummaryDlg;
    std::vector<std::tuple<std::unique_ptr<std::wstring>, std::wstring>> m_errors;
};
