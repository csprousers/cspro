#pragma once

#include <zConcatO/ConcatenatorReporter.h>
#include <zCaseO/StringBasedCaseConstructionReporter.h>

class ProcessSummaryDlg;


class CSConcatReporter : public ConcatenatorReporter, public StringBasedCaseConstructionReporter
{
public:
    CSConcatReporter(ProcessSummaryDlg& process_summary_dlg, std::shared_ptr<ProcessSummary> process_summary);

    // the tuple is the case key, if defined, and then the error
    const std::vector<std::tuple<std::unique_ptr<std::string>, std::string>>& GetErrors() const { return m_errors; }

    bool IsCanceled() const override;

    void SetSource(const std::string& source_text) override;
    void SetKey(const std::string& key) override;

    void ErrorFileOpenFailed(const std::string& file_path) override;
    void ErrorDataSourceOpenFailed(const ConnectionString& connection_string, const std::string& error_message) override;
    void ErrorInvalidEncoding(const std::string& file_path) override;
    void ErrorDuplicateCase(const std::string& key, const ConnectionString& connection_string, const ConnectionString& previous_connection_string) override;
    void ErrorOther(const ConnectionString& connection_string, const std::string& error_message) override;

protected:
    void WriteString(const std::string& key, std::string message) override;

private:
    ProcessSummaryDlg& m_processSummaryDlg;
    std::vector<std::tuple<std::unique_ptr<std::string>, std::string>> m_errors;
};
