#include "stdafx.h"
#include "GuiConcatenator.h"
#include <zToolsO/TextFile.h>
#include <zUtilO/ProcessSummary.h>
#include <zUtilF/ProcessSummaryDlg.h>
#include <zAppO/PFF.h>

using namespace Paradata;


int64_t GuiConcatenator::GetNumberEvents(const std::string& file_path)
{
    // read the number of events, aborting on any error
    int64_t events = -1;

    sqlite3* db = nullptr;

    if( PortableFunctions::FileIsRegular(file_path) &&
        sqlite3_open_v2(file_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK )
    {
        try
        {
            events = Concatenator::GetNumberEvents(db);
        }
        catch(...) { }

        sqlite3_close(db);
    }

    return events;
}


GuiConcatenator::GuiConcatenator(const PFF& pff)
    :   m_pff(pff),
        m_processSummaryDlg(nullptr)
{
}


bool GuiConcatenator::Run(const PFF& pff)
{
    return GuiConcatenator(pff).Run();
}


bool GuiConcatenator::Run()
{
    // open the log file
    if( m_pff.GetListingFName().IsEmpty() )
        throw CSProException("You must specify a listing file.");

    FileIO::TextFile log;

    try
    {
        log.OpenForTextWritingCreate(m_pff.GetListingFName());
    }

    catch( const CSProException& exception )
    {
        throw CSProException("There was an error creating the listing file:\n\n%s", exception.what());
    }

    log.WriteFormattedLine("Number of paradata logs requested to concatenate: %d", static_cast<int>(m_pff.GetInputParadataFilenames().size()));

    bool run_success = false;
    bool run_canceled = false;

    try
    {
        // display a progress bar while doing the concatenation
        m_processSummary = std::make_unique<ProcessSummary>();

        ProcessSummaryDlg process_summary_dlg;
        m_processSummaryDlg = &process_summary_dlg;

        process_summary_dlg.SetTask([&]
        {
            process_summary_dlg.Initialize("Concatenating...", m_processSummary);

            // run the concatenation
            std::set<std::string> paradata_log_file_paths;

            for( const CString& file_path : m_pff.GetInputParadataFilenames() )
                paradata_log_file_paths.insert(UTF8_TODO::GetUtf8(file_path));

            Concatenator::Run(UTF8_TODO::GetUtf8(m_pff.GetOutputParadataFilename()), paradata_log_file_paths);

            run_success = true;
        });

        process_summary_dlg.DoModal();

        process_summary_dlg.RethrowTaskExceptions();
    }

    catch( const CSProException& exception )
    {
        if( dynamic_cast<const UserCanceledException*>(&exception) != nullptr )
        {
            run_canceled = true;
        }

        else
        {
            log.WriteLine();
            log.WriteLine(exception.what());
        }
    }

    // terminate the listing file
    if( run_canceled )
    {
        log.WriteLine();
        log.WriteLine("Concatenation canceled");
    }

    else if( !run_success )
    {
        log.WriteLine();
        log.WriteLine("Concatenation failed");
    }

    else
    {
        if( !m_processedSuccesses.empty() )
        {
            log.WriteLine();
            log.WriteFormattedLine("Number of paradata logs successfully concatenated: %d", static_cast<int>(m_processedSuccesses.size()));

            for( const ProcessedSuccess& processed_success : m_processedSuccesses )
            {
                log.WriteFormattedLine("  %s (" Formatter_int64_t " event%s)", processed_success.file_path.c_str(),
                                                                               processed_success.number_events, PluralizeWord(processed_success.number_events));

            }
        }

        if( !m_processedErrors.empty() )
        {
            log.WriteLine();
            log.WriteFormattedLine("Number of paradata logs with errors: %d", static_cast<int>(m_processedErrors.size()));

            for( const ProcessedError& processed_error : m_processedErrors )
                log.WriteFormattedLine("  %s (%s)", processed_error.file_path.c_str(), processed_error.error_message.c_str());
        }

        log.WriteLine();
        log.WriteFormattedLine("Output paradata log:\n  %s", UTF8_TODO::GetUtf8(m_pff.GetOutputParadataFilename()).c_str());

        log.WriteLine();
        log.WriteLine(m_processedErrors.empty() ? "Concatenation successful" :
                                                  "Concatenation successful with some errors");
    }

    // close the log and potentially view the listing
    log.Close();

    const bool errors_occurred = ( !run_success || !m_processedErrors.empty() );

    if( ( m_pff.GetViewListing() == VIEWLISTING::ALWAYS ) ||
        ( m_pff.GetViewListing() == VIEWLISTING::ONERROR && errors_occurred ) )
    {
        m_pff.ViewListing();
    }

    return !errors_occurred;
}


void GuiConcatenator::OnInputProcessedSuccess(const std::variant<std::string, sqlite3*>& file_path_or_database, const int64_t events_processed)
{
    ASSERT(std::holds_alternative<std::string>(file_path_or_database));
    m_processedSuccesses.emplace_back(ProcessedSuccess { std::get<std::string>(file_path_or_database), events_processed });

    m_processSummary->IncrementAttributesRead(static_cast<size_t>(events_processed));
}


void GuiConcatenator::OnInputProcessedError(const std::string& input_file_path, const char* const error_message)
{
    m_processedErrors.emplace_back(ProcessedError { input_file_path, error_message });
}


void GuiConcatenator::OnProgressUpdate(const std::string& operation_message, const int operation_percent, const char* const total_message, const int total_percent)
{
    m_processSummaryDlg->SetSource(FormatText("%s (%d%%)...", total_message, operation_percent));
    m_processSummaryDlg->SetKey(operation_message);
    m_processSummary->SetPercentSourceRead(total_percent);
}


bool GuiConcatenator::UserRequestsCancellation()
{
    return m_processSummaryDlg->IsCanceled();
}
