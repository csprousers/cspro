#include "stdafx.h"
#include "Concatenator.h"
#include "CaseConcatenator.h"
#include "CSConcatReporter.h"
#include "TextConcatenator.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zUtilF/ProcessSummaryDlg.h>
#include <zAppO/PFF.h>


Concatenator::RunSuccess Concatenator::Run(const PFF& pff, const bool silent, std::shared_ptr<const CDataDict> dictionary/* = nullptr*/)
{
    //  open the log file
    if( pff.GetListingFName().IsEmpty() )
        throw CSProException("You must specify a listing filename.");

    CStdioFileUnicode log;

    if( !log.Open(pff.GetListingFName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText) )
    {
        throw CSProException(_T("There was an error creating the listing file:\n\n%s"),
                             pff.GetListingFName().GetString());
    }

    RunSuccess run_success = RunSuccess::Errors;

    try
    {
        // check that all of the file associations are properly set
        if( !pff.UsingOutputData() )
            throw CSProException("You must specify an output filename.");

        if( pff.GetInputDataConnectionStrings().empty() )
            throw CSProException("You must specify at least one file to concatenate.");

        // load the dictionary (if necessary)
        if( pff.GetConcatenateMethod() == ConcatenateMethod::Case && dictionary == nullptr )
        {
            if( pff.GetInputDictFName().IsEmpty() )
                throw CSProException("You must specify a dictionary when concatenating cases.");

            dictionary = CDataDict::InstantiateAndOpen(pff.GetInputDictFName(), silent);
        }


        // write the header
        log.WriteFormattedString(_T("Number of files requested to concatenate:  %d\n\n"),
                                 static_cast<int>(pff.GetInputDataConnectionStrings().size()));


        // run the concatenation
        ProcessSummaryDlg process_summary_dlg;

        process_summary_dlg.SetTask([&]
        {
            // the Run method will determine the concatenation type based on the presence of the dictionary,
            // so set it to null if this is a text concatenation
            if( pff.GetConcatenateMethod() == ConcatenateMethod::Text )
                dictionary = nullptr;

            std::shared_ptr<ProcessSummary> process_summary = ( dictionary != nullptr ) ? dictionary->CreateProcessSummary() :
                                                                                          std::make_shared<ProcessSummary>();
            auto csconcat_reporter = std::make_shared<CSConcatReporter>(process_summary_dlg, process_summary);
            process_summary_dlg.Initialize(_T("Concatenating Files..."), process_summary);

            Run(*csconcat_reporter, pff.GetInputDataConnectionStrings(), pff.GetSingleOutputDataConnectionString(), dictionary, csconcat_reporter);

            // write the summary information
            for( const ConnectionString& connection_string : csconcat_reporter->GetSuccessfullyProcessedFiles() )
                log.WriteFormattedString(_T("  %s\n"), connection_string.GetFilename().c_str());

            log.WriteFormattedString(_T("\nNumber of files concatenated:  %d\n\n"),
                                     static_cast<int>(csconcat_reporter->GetSuccessfullyProcessedFiles().size()));

            log.WriteFormattedString(_T("Output file:\n  %s\n"),
                                     pff.GetSingleOutputDataConnectionString().GetFilename().c_str());

            if( csconcat_reporter->GetErrors().empty() )
            {
                run_success = RunSuccess::Success;
                log.WriteString(_T("\n\nConcatenation successful.\n"));
            }

            else
            {
                run_success = RunSuccess::SuccessWithErrors;
                log.WriteString(_T("\n\nConcatenation done with the following errors:\n\n"));

                for( const auto& [key, message] : csconcat_reporter->GetErrors() )
                {
                    if( key != nullptr )
                    {
                        log.WriteFormattedString(_T("*** [%s]\n"),
                                                 NewlineSubstitutor::NewlineToUnicodeNL(*key).c_str());
                    }

                    log.WriteFormattedString(_T("*** %s\n\n"), message.c_str());
                }
            }
        });

        process_summary_dlg.DoModal();

        process_summary_dlg.RethrowTaskExceptions();
    }

    catch( const CSProException& exception )
    {
        if( dynamic_cast<const UserCanceledException*>(&exception) != nullptr )
            run_success = RunSuccess::UserCanceled;

        log.WriteFormattedString(_T("*** %s"), exception.GetErrorMessage().c_str());
        log.WriteString(_T("\n\nConcatenation failed.\n"));
    }

    // close the log and potentially view the listing and results
    log.Close();

    if( pff.GetViewListing() == ALWAYS || ( pff.GetViewListing() == ONERROR && run_success != RunSuccess::Success ) )
        pff.ViewListing();

    if( pff.GetViewResultsFlag() && ( run_success == RunSuccess::Success || run_success == RunSuccess::SuccessWithErrors ) )
        PFF::ViewResults(pff.GetSingleOutputDataConnectionString());

    return run_success;
}


void Concatenator::Run(ConcatenatorReporter& concatenator_reporter,
                       const std::vector<ConnectionString>& input_connection_strings, const ConnectionString& output_connection_string,
                       std::shared_ptr<const CDataDict> dictionary/* = nullptr*/, std::shared_ptr<CaseConstructionReporter> case_construction_reporter/* = nullptr*/)
{
    if( dictionary == nullptr )
    {
        TextConcatenator().Run(concatenator_reporter, input_connection_strings, output_connection_string);
    }

    else
    {
        CaseConcatenator().Run(concatenator_reporter, input_connection_strings, output_connection_string,
                               std::move(dictionary), std::move(case_construction_reporter));
    }
}
