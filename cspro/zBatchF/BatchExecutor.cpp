#include "StdAfx.h"
#include "BatchExecutor.h"
#include "BatchExecutionDlg.h"
#include <zEngineO/ApplicationBuilder.h>
#include <zEngineO/FileApplicationLoader.h>
#include <zFormO/FormFile.h> // BATCH_FLOW_TODO remove, also may not need references to zDictO+zFormO


BatchExecutor::BatchExecutor(BatchExecutorCallback* const batch_executor_callback/* = nullptr*/)
    :   m_batchExecutorCallback(batch_executor_callback)
{
}


void BatchExecutor::AddUWMCallback(const unsigned message, std::shared_ptr<UWMCallback> uwm_callback)
{
    m_uwmCallbacks[message] = std::move(uwm_callback);
}


void BatchExecutor::Run(const std::string& pff_or_batch_file_path)
{
    std::string batch_file_path;
    std::string pff_file_path;
    bool user_specified_pff = false;
    bool pff_launched_from_command_line = false;

    auto set_filenames_from_input_file_path = [&](std::string file_path)
    {
        if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), FileExtensions::Pff) )
        {
            pff_file_path = std::move(file_path);
            user_specified_pff = true;
        }

        else
        {
            batch_file_path = std::move(file_path);
        }
    };

    // if the file path was provided, then we do not have to query for one
    if( PortableFunctions::FileIsRegular(pff_or_batch_file_path) )
    {
        set_filenames_from_input_file_path(pff_or_batch_file_path);
        pff_launched_from_command_line = user_specified_pff;
    }

    else
    {
        std::string queried_file_path;

        if( m_batchExecutorCallback == nullptr || !m_batchExecutorCallback->QueryForFilePath(queried_file_path) )
            return;

        set_filenames_from_input_file_path(std::move(queried_file_path));
    }


    // load the PFF...
    CNPifFile pff;

    auto load_pff = [&]
    {
        pff.SetPifFileName(UTF8_TODO::GetCString(pff_file_path));

        if( !pff.LoadPifFile() )
            throw CSProException("The was an error loading: %s", pff_file_path.c_str());
    };

    // if we have a PFF file path, then we need to get the batch file path for the PFF
    if( user_specified_pff )
    {
        load_pff();
        batch_file_path = UTF8_TODO::GetUtf8(pff.GetAppFName());
    }

    // otherwise see if there is an existing PFF for this batch application
    else
    {
        pff_file_path = PortableFunctions::PathReplaceFileExtension(batch_file_path, FileExtensions::Pff);

        if( PortableFunctions::FileIsRegular(pff_file_path) )
        {
            load_pff();

            if( !SO::EqualsNoCase(batch_file_path, pff.GetAppFName()) )
            {
                throw CSProException("The default PFF for this application (%s) is associated with a different application (%s). "
                                     "Delete it or specify a PFF to run.",
                                     PortableFunctions::PathGetFilename(batch_file_path).c_str(),
                                     PortableFunctions::PathGetFilename(UTF8_TODO::GetUtf8(pff.GetAppFName())).c_str());
            }
        }

        // otherwise create a new batch PFF
        else
        {
            pff.SetPifFileName(UTF8_TODO::GetCString(pff_file_path));
            pff.SetAppType(BATCH_TYPE);
            pff.SetAppFName(UTF8_TODO::GetCString(batch_file_path));
            pff.SetViewListing(ALWAYS);
            pff.SetViewResultsFlag(true);
        }
    }


    // set the current directory to the location of the batch file
    SetCurrentDirectory(TC::ToWide(PortableFunctions::PathGetDirectory(batch_file_path)).c_str());


    // build the application and then pass control of it to the PFF object
    auto application = std::make_shared<Application>();
    pff.SetApplication(application);

    BuildApplication(std::make_unique<FileApplicationLoader>(application.get(), batch_file_path), EngineAppType::Batch);

    const bool BATCH_FLOW_TODO_use_new_batch_driver =
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
    for( const ConnectionString& output_data_connection_string : pff.GetOutputDataConnectionStrings() )
    {
        if( output_data_connection_string.HasFilePath() )
            CloseFileInTextViewer(output_data_connection_string.GetFilePath(), true);
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
        pff.ViewResults(UTF8_TODO::GetUtf8(pff.GetFrequenciesFilename()));
        pff.ViewResults(UTF8_TODO::GetUtf8(pff.GetImputeFrequenciesFilename()));
        ViewFileInTextViewer(pff.GetWriteFName());
    }

    if( pff.GetViewListing() != NEVER )
    {
        ViewFileInTextViewer(pff.GetApplicationErrorsFilename());

        bool show_listing = true;

        if( pff.GetViewListing() == ONERROR )
        {
            const std::shared_ptr<const ProcessSummary> process_summary = batch_execution_dlg->GetProcessSummary();
            show_listing = ( process_summary != nullptr && process_summary->GetTotalMessages() != 0 );
        }

        if( show_listing )
            Listing::Lister::View(UTF8_TODO::GetUtf8(pff.GetListingFName()));
    }


    // run the OnExit
    if( pff_launched_from_command_line )
        pff.ExecuteOnExitPff();
}
