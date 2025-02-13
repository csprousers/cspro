#include "StdAfx.h"
#include "BatchExecutor.h"
#include "BatchExecutionDlg.h"
#include <zEngineO/ApplicationBuilder.h>
#include <zEngineO/FileApplicationLoader.h>
#include <zFormO/FormFile.h> // BATCH_FLOW_TODO remove, also may not need references to zDictO+zFormO


BatchExecutor::BatchExecutor(BatchExecutorCallback* batch_executor_callback/* = nullptr*/)
    :   m_batchExecutorCallback(batch_executor_callback)
{
}


void BatchExecutor::AddUWMCallback(unsigned message, std::shared_ptr<UWMCallback> uwm_callback)
{
    m_uwmCallbacks[message] = uwm_callback;
}


void BatchExecutor::Run(const CString& pff_or_batch_filename)
{
    CString batch_filename;
    CString pff_filename;
    bool user_specified_pff = false;
    bool pff_launched_from_command_line = false;

    auto set_filenames_from_input_filename = [&](const CString& filename)
    {
        if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(filename), FileExtensions::Pff) )
        {
            pff_filename = filename;
            user_specified_pff = true;
        }

        else
        {
            batch_filename = filename;
        }
    };

    // if the filename was provided, then we do not have to query for one
    if( PortableFunctions::FileIsRegular(pff_or_batch_filename) )
    {
        set_filenames_from_input_filename(pff_or_batch_filename);
        pff_launched_from_command_line = user_specified_pff;
    }

    else
    {
        CString queried_filename;

        if( m_batchExecutorCallback == nullptr || !m_batchExecutorCallback->QueryForFilename(queried_filename) )
            return;

        set_filenames_from_input_filename(queried_filename);
    }


    // load the PFF...
    CNPifFile pff;

    auto load_pff = [&]
    {
        pff.SetPifFileName(pff_filename);

        if( !pff.LoadPifFile() )
            throw CSProException(_T("The was an error loading: %s"), pff_filename.GetString());
    };

    // if we have a PFF filename, then we need to get the batch filename for the PFF
    if( user_specified_pff )
    {
        load_pff();
        batch_filename = pff.GetAppFName();
    }

    // otherwise see if there is an existing PFF for this batch application
    else
    {
        pff_filename = PortableFunctions::PathRemoveFileExtension<CString>(batch_filename) + FileExtensions::WithDot::Pff;

        if( PortableFunctions::FileIsRegular(pff_filename) )
        {
            load_pff();

            if( batch_filename.CompareNoCase(pff.GetAppFName()) != 0 )
            {
                throw CSProException(_T("The default PFF for this application (%s) is associated with a different application (%s). ")
                                     _T("Delete it or specify a PFF to run."),
                                     PortableFunctions::PathGetFilename(batch_filename),
                                     PortableFunctions::PathGetFilename(pff.GetAppFName()));
            }
        }

        // otherwise create a new batch PFF
        else
        {
            pff.SetPifFileName(pff_filename);
            pff.SetAppType(BATCH_TYPE);
            pff.SetAppFName(batch_filename);
            pff.SetViewListing(ALWAYS);
            pff.SetViewResultsFlag(true);
        }
    }


    // set the current directory to the location of the batch file
    SetCurrentDirectory(PortableFunctions::PathGetDirectory(batch_filename).c_str());


    // build the application and then pass control of it to the PFF object
    auto application = std::make_shared<Application>();
    pff.SetApplication(application);

    BuildApplication(std::make_shared<FileApplicationLoader>(application.get(), batch_filename), EngineAppType::Batch);

    bool BATCH_FLOW_TODO_use_new_batch_driver =
        !application->GetRuntimeFormFiles().empty() &&
        application->GetRuntimeFormFiles().front()->GetDictionary() != nullptr &&
        application->GetRuntimeFormFiles().front()->GetDictionary()->UseNewSymbols();

    std::unique_ptr<BatchDriver> batch_driver;
    std::unique_ptr<CRunAplBatch> batch_application;

    if( BATCH_FLOW_TODO_use_new_batch_driver )
    {
        // create the batch driver, which will also compile the application
        batch_driver = BatchDriver::Create(pff);
    }

    else
    {
        // compile the rest of the application
        batch_application = std::make_unique<CRunAplBatch>(&pff);

        if( !batch_application->LoadCompile() )
        {
            batch_application->End(false);
            throw CSProException("There was an error compiling the batch application");
        }

        application->SetCompiled(true);
    }


    // show the file assocations dialog if necessary
    if( !user_specified_pff )
    {
        if( BATCH_FLOW_TODO_use_new_batch_driver )
        {
            if( m_batchExecutorCallback == nullptr || !m_batchExecutorCallback->QueryForFileAssociations(pff, batch_driver->GetEngineData()) )
                return;
        }

        else
        {
            if( m_batchExecutorCallback == nullptr || !m_batchExecutorCallback->QueryForFileAssociations(pff, *batch_application->GetEngineArea()->m_engineData) )
                return;
        }
    }


    // close and delete several auxiliary files
    for( const auto& output_data_connection_string : pff.GetOutputDataConnectionStrings() )
    {
        if( output_data_connection_string.IsFilenamePresent() )
            CloseFileInTextViewer(output_data_connection_string.GetFilename(), true);
    }

    CloseFileInTextViewer(pff.GetListingFName(), true);
    CloseFileInTextViewer(pff.GetWriteFName(), true);
    CloseFileInTextViewer(pff.GetImputeFrequenciesFilename(), true);


    // the batch execution dialog will run the engine in a background thread
    std::unique_ptr<BatchExecutionDlg> batch_execution_dlg;

    if( BATCH_FLOW_TODO_use_new_batch_driver )
    {
        batch_execution_dlg = std::make_unique<BatchExecutionDlg>(pff, batch_driver.get(), m_uwmCallbacks);
    }

    else
    {
        // finish setting up the batch application
        batch_application->SetBatchMode(CRUNAPL_CSBATCH);

        // if no input file is specified, have a null repository as the input
        if( pff.GetInputDataConnectionStrings().empty() )
            pff.SetSingleInputDataConnectionString(ConnectionString::CreateNullRepositoryConnectionString());

        batch_execution_dlg = std::make_unique<BatchExecutionDlg>(pff, batch_application.get(), m_uwmCallbacks);
    }

    batch_execution_dlg->DoModal();


    // view the results and the listing
    if( pff.GetViewResultsFlag() )
    {
        pff.ViewResults(pff.GetFrequenciesFilename());
        pff.ViewResults(pff.GetImputeFrequenciesFilename());
        ViewFileInTextViewer(pff.GetWriteFName());
    }

    if( pff.GetViewListing() != NEVER )
    {
        ViewFileInTextViewer(pff.GetApplicationErrorsFilename());

        bool show_listing = true;

        if( pff.GetViewListing() == ONERROR )
        {
            auto process_summary = batch_execution_dlg->GetProcessSummary();
            show_listing = ( process_summary != nullptr && process_summary->GetTotalMessages() != 0 );
        }

        if( show_listing )
            Listing::Lister::View(pff.GetListingFName());
    }


    // run the OnExit
    if( pff_launched_from_command_line )
        pff.ExecuteOnExitPff();
}
