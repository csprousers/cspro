#include "stdafx.h"
#include "CSConcatReporter.h"
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


void CSConcatReporter::SetSource(const NullTerminatedString source)
{
    m_processSummaryDlg.SetSource(source);
}


void CSConcatReporter::SetKey(const NullTerminatedString key)
{
    m_processSummaryDlg.SetKey(key);
}
    
    
void CSConcatReporter::ErrorFileOpenFailed(const ConnectionString& connection_string)
{
    m_errors.emplace_back(nullptr, FormatTextCS2WS(_T("Unable to open file: %s"),
                                                   connection_string.GetFilename().c_str()));
}


void CSConcatReporter::ErrorInvalidEncoding(const ConnectionString& connection_string)
{
    m_errors.emplace_back(nullptr, FormatTextCS2WS(_T("Text file is not encoded in a format supported by CSPro: %s"),
                                                   connection_string.GetFilename().c_str()));
}


void CSConcatReporter::ErrorDuplicateCase(const NullTerminatedString key, const ConnectionString& connection_string, const ConnectionString& previous_connection_string)
{
    // the case key isn't added to m_errors because we display it in the message
    m_errors.emplace_back(nullptr, FormatTextCS2WS(_T("Skipping duplicate case '%s' in file '%s'. Previously found in file '%s'."),
                                                   key.c_str(),
                                                   connection_string.GetFilename().c_str(),
                                                   previous_connection_string.GetFilename().c_str())); 
}


void CSConcatReporter::ErrorOther(const ConnectionString& connection_string, const NullTerminatedString error_message)
{
    m_errors.emplace_back(nullptr, FormatTextCS2WS(_T("Error concatenating cases from file '%s': %s. Remaining cases in this file will be skipped."),
                                                   connection_string.GetFilename().c_str(),
                                                   error_message.c_str()));
}


void CSConcatReporter::WriteString(const NullTerminatedString key, const NullTerminatedString message)
{
    m_errors.emplace_back(std::make_unique<std::wstring>(key), message);
}
