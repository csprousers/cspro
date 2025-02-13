#include "StdAfx.h"
#include "BatchDriver.h"
#include <zEngineO/EngineDictionary.h>
#include <engine/Engine.h>


BatchDriver::BatchDriver(CNPifFile& pff)
    :   CEngineDriver(&pff),
        m_processSummaryReporter(nullptr),
        m_engineCase(nullptr)
{
}


BatchDriver::~BatchDriver()
{
    if( m_pEngineArea != nullptr )
        m_pEngineArea->tablesend();
}


std::unique_ptr<BatchDriver> BatchDriver::Create(CNPifFile& pff)
{
    ASSERT(pff.GetAppType() == APPTYPE::BATCH_TYPE && pff.GetApplication() != nullptr);

    Application& application = *pff.GetApplication();
    ASSERT(!application.IsCompiled());
    ASSERT(application.GetApplicationLoader() != nullptr);

    std::unique_ptr<BatchDriver> batch_driver(new BatchDriver(pff));

    batch_driver->m_Issamod = ModuleType::Batch;
    batch_driver->m_lpszExecutorLabel =_T("BATCH");

    if( !batch_driver->exapplinit() )
        throw CSProException("There was an error compiling the batch application.");

    application.SetCompiled(true);

    return batch_driver;
}


void BatchDriver::Run()
{
    if( InitializeRun() )
    {
        // start the session and run the application preproc
        m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::SessionStart);

        ExecuteApplicationProc(ProcType::PreProc);

        // only continue with the main session if the preproc executed without stop being called
        if( !m_pIntDriver->m_bStopProc )
            RunBatchOnInputs();

        ExecuteApplicationProc(ProcType::PostProc);

        m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::SessionStop);
    }

    FinalizeRun();
}


bool BatchDriver::InitializeRun()
{
    // set the primary flow
    // FLOW_TODO add back if needed m_pEngineDriver->SetFlowInProcess(Appl.GetFlowAt(0));
    m_pEngineSettings->SetPathOff();

    try
    {
        OpenListerAndWriteFiles();

        m_pEngineDriver->GetLister()->SetMessageSource(SO::Concatenate(m_lpszExecutorLabel, _T(" INITIALIZATION")));

        m_pIntDriver->StartApplication();


        // open the repositories
        if( !OpenRepositories(false) )
            return false;

        // open the output repositories
        if( m_pPifFile->UsingOutputData() && !m_pEngineDriver->OpenBatchOutputRepositories(m_pPifFile->GetOutputDataConnectionStrings(), false) )
            return false;


        // initialize sections for each dictionary
        initwsect();

        for( DICT* pDicT : m_engineData->dictionaries_pre80 )
        {
            if( pDicT->GetSubType() == SymbolSubType::Input || pDicT->GetSubType() == SymbolSubType::Work )
                continue;

            for( int iSymSec = pDicT->SYMTfsec; iSymSec > 0; iSymSec = SPT(iSymSec)->SYMTfwd )
                initsect(iSymSec);
        }


        // initialize the process summary reporter
        AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_GET_PROCESS_SUMMARY_REPORTER, (WPARAM)&m_processSummaryReporter);
        ASSERT(m_processSummaryReporter != nullptr);

        CString dialog_title = FormatText(_T("Running %s application %s. Press ESC to interrupt..."),
                                          m_lpszExecutorLabel,
                                          PortableFunctions::PathGetFilename(Appl.GetAppFileName()));

        m_processSummaryReporter->Initialize(dialog_title, m_pEngineDriver->GetProcessSummary(), &m_pIntDriver->m_bStopProc);
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Abort, exception);
        return false;
    }

    return true;
}


void BatchDriver::FinalizeRun()
{
    // close the repositories
    CloseRepositories(false);
    CloseBatchOutputRepositories();

    m_pIntDriver->StopApplication();

    m_pEngineDriver->CloseListerAndWriteFiles();

    // reset the flow
    // FLOW_TODO m_pEngineDriver->ResetFlowInProcess();
}


void BatchDriver::ExecuteApplicationProc(ProcType proc_type)
{
    const TCHAR* proc_type_text;

    if( proc_type == ProcType::PreProc )
    {
        ASSERT(!m_pIntDriver->m_bStopProc);

        proc_type_text = _T("PREPROC");
    }

    else
    {
        ASSERT(proc_type == ProcType::PostProc);

        proc_type_text = _T("POSTPROC");

        // if processing was stopped using either stop without (1) or by hitting cancel on the
        // batch meter dialog, turn off the stop flag so that the application postproc runs
        if( m_pIntDriver->m_bStopProc && m_pEngineDriver->GetStopCode() != 1 )
            m_pIntDriver->m_bStopProc = false;
    }


    CString proc_info = FormatText(_T("LEVEL 0 %s"), proc_type_text);
    m_processSummaryReporter->SetSource(proc_info);
    m_pEngineDriver->GetLister()->SetMessageSource(CS2WS(proc_info));

    m_pIntDriver->ExecuteProcLevel(0, proc_type);    
}


void BatchDriver::RunBatchOnInputs()
{
    // gather information on all of the repositories that will be processed
    std::vector<std::tuple<ConnectionString, double>> input_connection_strings_and_file_sizes;
    double total_file_size = 0;

    for( const auto& input_connection_string : m_pPifFile->GetInputDataConnectionStrings() )
    {
        // use a dummy file size in instances when the file size cannot be calculated or when the file is empty/small
        double file_size = 100;

        if( input_connection_string.IsFilenamePresent() )
            file_size = std::max(file_size, (double)PortableFunctions::FileSize(input_connection_string.GetFilename()));

        input_connection_strings_and_file_sizes.emplace_back(input_connection_string, file_size);

        total_file_size += file_size;
    }


    // process each repository
    int multiple_repository_index = 0;

    for( const auto& [ input_connection_string, file_size ] : input_connection_strings_and_file_sizes )
    {
        CString source_text = _T("File");

        if( input_connection_strings_and_file_sizes.size() > 1 )
            source_text.AppendFormat(_T(" %d of %d"), ++multiple_repository_index, (int)input_connection_strings_and_file_sizes.size());

        if( input_connection_string.IsFilenamePresent() )
            source_text.AppendFormat(_T(": %s"), PortableFunctions::PathGetFilename(input_connection_string.GetFilename()));

        m_processSummaryReporter->SetSource(source_text);

        RunBatchOnInput(input_connection_string, file_size / total_file_size);

        if( m_pIntDriver->m_bStopProc )
            break;
    }


    // if the user didn't cancel, set the percent read to 100%; otherwise, make sure that the percent does
    // not end up as 100% (because some repositories don't report percents accurately if they read cases in advance)
    size_t final_percent = !m_pIntDriver->m_bStopProc ? 100 :
                                                        std::min(GetProcessSummary()->GetPercentSourceRead(), (size_t)99);

    GetProcessSummary()->SetPercentSourceRead(final_percent);
}


void BatchDriver::RunBatchOnInput(const ConnectionString& input_connection_string, double repository_percent_fraction)
{
    size_t process_update_frequency = 10;
    size_t cases_until_progress_update = 1;
    size_t last_percent_read = SIZE_MAX;
    std::shared_ptr<ProcessSummary> process_summary = GetProcessSummary();

    try
    {
        // open the repository
        EngineDictionary& input_engine_dictionary = *m_engineData->engine_dictionaries.front();
        EngineDataRepository& input_engine_data_repository = input_engine_dictionary.GetEngineDataRepository();
        m_engineCase = &input_engine_dictionary.GetEngineCase();

        m_pEngineDriver->OpenRepository(input_engine_data_repository, input_connection_string, DataRepositoryOpenFlag::OpenMustExist, false);

        // iterate over each case
        while( input_engine_data_repository.StepCaseIterator(*m_engineCase) )
        {
            m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStart);

            RunBatchOnCase();

            m_pIntDriver->m_pParadataDriver->LogEngineEvent(ParadataEngineEvent::CaseStop);

            if( m_pIntDriver->m_bStopProc )
                break;


            // update the progress bar
            if( --cases_until_progress_update == 0 )
            {
                size_t percent_read = (size_t)( input_engine_data_repository.GetCaseIteratorPercentRead() * repository_percent_fraction );
                process_summary->SetPercentSourceRead(percent_read);

                // if processing a massive file, update the progress bar less frequently
                if( percent_read == last_percent_read )
                    process_update_frequency = (size_t)( process_update_frequency * 1.2 );

                else
                    last_percent_read = percent_read;

                cases_until_progress_update = process_update_frequency;

                m_processSummaryReporter->SetKey(m_engineCase->GetCase().GetKey());
            }
        }


        // close the repository
        input_engine_data_repository.CloseDataRepository();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Error, 10105, exception.GetErrorMessage().c_str());
    }
}


void BatchDriver::RunBatchOnCase()
{
    ASSERT(m_engineCase != nullptr);

    bool write_case = true; // BATCH_FLOW_TODO .. run procs

    if( !write_case )
        return;

    // write to the output repositories
    for( const auto& output_data_repository : m_batchOutputRepositories )
        output_data_repository->WriteCase(m_engineCase->GetCase());

    // BATCH_FLOW_TODO write to special outputs
}
