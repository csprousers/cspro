#include "stdafx.h"
#include "CSConcatReporter.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilF/ProcessSummaryDlg.h>


CSConcatReporter::CSConcatReporter(ProcessSummaryDlg& process_summary_dlg, std::shared_ptr<ProcessSummary> process_summary)
    :   ConcatenatorReporter(process_summary),
        StringBasedCaseConstructionReporter(std::move(process_summary)),
        m_processSummaryDlg(process_summary_dlg)
{
}


bool CSConcatReporter::IsCanceled() const
{
    return m_processSummaryDlg.IsCanceled();
}


void CSConcatReporter::SetSource(const std::string& source_text)
{
    m_processSummaryDlg.SetSource(source_text);
}


void CSConcatReporter::SetKey(const std::string& key)
{
    m_processSummaryDlg.SetKey(key);
}


void CSConcatReporter::ErrorFileOpenFailed(const std::string& file_path)
{
    m_errors.emplace_back(nullptr, "Unable to open file: " +  file_path);
}


void CSConcatReporter::ErrorDataSourceOpenFailed(const ConnectionString& connection_string, const std::string& error_message)
{
    m_errors.emplace_back(nullptr, "Unable to open data source: " + SO::CreateParentheticalExpression(connection_string.ToDisplayString(), error_message));
}


void CSConcatReporter::ErrorInvalidEncoding(const std::string& file_path)
{
    m_errors.emplace_back(nullptr, "Text file is not encoded in a format supported by CSPro: " + file_path);
}


void CSConcatReporter::ErrorDuplicateCase(const std::string& key, const ConnectionString& connection_string, const ConnectionString& previous_connection_string)
{
    // the case key isn't added to m_errors because we display it in the message
    m_errors.emplace_back(nullptr, FormatText("Skipping duplicate case '%s' in data source '%s'. Previously found in data source '%s'.",
                                              NewlineSubstitutor::NewlineToUnicodeNL(key).c_str(),
                                              connection_string.ToDisplayString().c_str(),
                                              previous_connection_string.ToDisplayString().c_str()));
}


void CSConcatReporter::ErrorOther(const ConnectionString& connection_string, const std::string& error_message)
{
    m_errors.emplace_back(nullptr, FormatText("Error concatenating cases from data source '%s': %s. Remaining cases in this data source will be skipped.",
                                              connection_string.ToDisplayString().c_str(),
                                              error_message.c_str()));
}


void CSConcatReporter::WriteString(const std::string& key, std::string message)
{
    m_errors.emplace_back(std::make_unique<std::string>(key), std::move(message));
}
