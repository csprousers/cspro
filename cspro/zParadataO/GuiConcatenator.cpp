#include "stdafx.h"
#include "GuiConcatenator.h"
#include <SQLite/SQLiteHelpers.h>
#include <zUtilO/ProcessSummary.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilF/ProcessSummaryDlg.h>


namespace Paradata
{
    int64_t GuiConcatenator::GetNumberEvents(NullTerminatedString filename)
    {
        // read the number of events, aborting on any error
        int64_t iEvents = -1;

        sqlite3* db = nullptr;

        if( PortableFunctions::FileIsRegular(filename) &&
            sqlite3_open_v2(ToUtf8(filename), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK )
        {
            try
            {
                iEvents = Concatenator::GetNumberEvents(db);
            }

            catch(...)
            {
            }

            sqlite3_close(db);
        }

        return iEvents;
    }


    GuiConcatenator::GuiConcatenator(const PffWrapper& pff)
        :   m_pff(pff),
            m_processSummaryDlg(nullptr)
    {
    }

    bool GuiConcatenator::Run(const PffWrapper& pff)
    {
        return GuiConcatenator(pff).Run();
    }


    bool GuiConcatenator::Run()
    {
        // open the log file
        if( m_pff.GetListingFName().IsEmpty() )
            throw CSProException("You must specify a listing filename.");

        CStdioFileUnicode log;

        if( !log.Open(m_pff.GetListingFName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText) )
            throw CSProException(_T("There was an error creating the listing file:\n\n%s"), m_pff.GetListingFName().GetString());

        log.WriteFormattedLine(_T("Number of paradata logs requested to concatenate: %d"), (int)m_pff.GetInputParadataFilenames().size());

        bool run_success = false;
        bool run_canceled = false;

        try
        {
            // display a progress bar while doing the concatenation
            m_processSummary = std::make_shared<ProcessSummary>();

            ProcessSummaryDlg process_summary_dlg;
            m_processSummaryDlg = &process_summary_dlg;

            process_summary_dlg.SetTask([&]
            {
                process_summary_dlg.Initialize(_T("Concatenating..."), m_processSummary);

                // run the concatenation
                std::set<std::wstring> paradata_log_filenames;

                for( const std::wstring& filename : m_pff.GetInputParadataFilenames() )
                    paradata_log_filenames.insert(filename);

                Concatenator::Run(CS2WS(m_pff.GetOutputParadataFilename()), paradata_log_filenames);

                run_success = true;
            });

            process_summary_dlg.DoModal();

            process_summary_dlg.RethrowTaskExceptions();
        }

        catch( const CSProException& exception )
        {
            if( dynamic_cast<const UserCanceledException*>(&exception) != nullptr )
                run_canceled = true;

            else
            {
                log.WriteLine();
                log.WriteLine(exception.GetErrorMessage());
            }
        }

        // terminate the listing file
        if( run_canceled )
        {
            log.WriteLine();
            log.WriteLine(_T("Concatenation canceled"));
        }

        else if( !run_success )
        {
            log.WriteLine();
            log.WriteLine(_T("Concatenation failed"));
        }

        else
        {
            if( !m_aProcessedSuccess.empty() )
            {
                log.WriteLine();
                log.WriteFormattedLine(_T("Number of paradata logs successfully concatenated: %d"), (int)m_aProcessedSuccess.size());

                for( const auto& processed_success : m_aProcessedSuccess )
                {
                    log.WriteFormattedLine(_T("  %s (") Formatter_int64_t _T(" event%s)"), processed_success.csFilename.GetString(),
                                           processed_success.iNumberEvents, PluralizeWord(processed_success.iNumberEvents));

                }
            }

            if( !m_aProcessedError.empty() )
            {
                log.WriteLine();
                log.WriteFormattedLine(_T("Number of paradata logs with errors: %d"), (int)m_aProcessedError.size());

                for( const auto& processed_error : m_aProcessedError )
                    log.WriteFormattedLine(_T("  %s (%s)"), processed_error.csFilename.GetString(), processed_error.csErrorMessage.GetString());
            }

            log.WriteLine();
            log.WriteFormattedLine(_T("Output paradata log:\n  %s"), m_pff.GetOutputParadataFilename().GetString());

            log.WriteLine();
            log.WriteLine(m_aProcessedError.empty() ? _T("Concatenation successful") :
                                                      _T("Concatenation successful with some errors"));
        }

        // close the log and potentially view the listing
        log.Close();

        bool errors_occurred = ( !run_success || !m_aProcessedError.empty() );

        m_pff.ViewListing(errors_occurred);

        return !errors_occurred;
    }


    void GuiConcatenator::OnInputProcessedSuccess(const std::variant<std::wstring, sqlite3*>& filename_or_database, int64_t iEventsProcessed)
    {
        ASSERT(std::holds_alternative<std::wstring>(filename_or_database));
        m_aProcessedSuccess.emplace_back(ProcessedSuccess { WS2CS(std::get<std::wstring>(filename_or_database)), iEventsProcessed });

        m_processSummary->IncrementAttributesRead((size_t)iEventsProcessed);
    }


    void GuiConcatenator::OnInputProcessedError(NullTerminatedString input_filename, const std::wstring& error_message)
    {
        m_aProcessedError.emplace_back(ProcessedError { input_filename, WS2CS(error_message) });
    }


    void GuiConcatenator::OnProgressUpdate(const CString& csOperationMessage, int iOperationPercent, const CString& csTotalMessage, int iTotalPercent)
    {
        m_processSummaryDlg->SetSource(FormatText(_T("%s (%d%%)..."), csTotalMessage.GetString(), iOperationPercent));
        m_processSummaryDlg->SetKey(csOperationMessage);
        m_processSummary->SetPercentSourceRead(iTotalPercent);
    }


    bool GuiConcatenator::UserRequestsCancellation()
    {
        return m_processSummaryDlg->IsCanceled();
    }
}
