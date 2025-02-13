#include "StdAfx.h"
#include "BatchExecutionDlg.h"
#include <zToolsO/UWM.h>
#include <zUtilF/UIThreadRunner.h>
#include <zListingO/Lister.h>
#include <engine/EngineObjectTransporter.h>


BEGIN_MESSAGE_MAP(BatchExecutionDlg, BatchMeterDlg)
    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)
    ON_MESSAGE(WM_IMSA_START_ENGINE, OnStartEngine)
    ON_MESSAGE(WM_IMSA_GET_PROCESS_SUMMARY_REPORTER, OnGetProcessSummaryReporter)
    ON_MESSAGE(WM_IMSA_ENGINEABORT, OnEngineAbort)
    ON_MESSAGE(WM_IMSA_PORTABLE_ENGINEUI, OnEngineUI)
    ON_MESSAGE(UWM::UtilF::RunOnUIThread, OnRunOnUIThread)
    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)
END_MESSAGE_MAP()


BatchExecutionDlg::BatchExecutionDlg(const CNPifFile& pff, std::variant<CRunAplBatch*, BatchDriver*> batch_driver,
    std::map<unsigned, std::shared_ptr<UWMCallback>> uwm_callbacks, CWnd* pParent/* = nullptr*/)
    :   BatchMeterDlg(pParent),
        m_pff(pff),
        m_batchDriver(batch_driver),
        m_uwmCallbacks(uwm_callbacks),
        m_engineUIProcessor(&m_pff, false)
{
}

BatchExecutionDlg::~BatchExecutionDlg()
{
    if( m_engineThread != nullptr && m_engineThread->joinable() )
        m_engineThread->join();
}


BOOL BatchExecutionDlg::OnInitDialog()
{
    BatchMeterDlg::OnInitDialog();

    PostMessage(WM_IMSA_START_ENGINE);

    return TRUE;
}


LRESULT BatchExecutionDlg::OnGetObjectTransporter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_objectTransporter == nullptr )
    {
        const CEngineArea* engine_area = std::visit([](const auto& batch_driver) { return batch_driver->GetEngineArea(); }, m_batchDriver);
        ASSERT(engine_area != nullptr);

        m_objectTransporter = std::make_unique<EngineObjectTransporter>(engine_area);
    }   

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


LRESULT BatchExecutionDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if( !m_uwmCallbacks.empty() )
    {
        const auto& uwm_callback_lookup = m_uwmCallbacks.find(message);

        if( uwm_callback_lookup != m_uwmCallbacks.cend() )
            return uwm_callback_lookup->second->ProcessMessage(wParam, lParam);
    }
    
    return BatchMeterDlg::WindowProc(message, wParam, lParam);
}


LRESULT BatchExecutionDlg::OnStartEngine(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_engineThread = std::make_unique<std::thread>([&]
    {
        // set the main window to this modal dialog
        CWnd* saved_main_window = AfxGetApp()->m_pMainWnd;
        AfxGetApp()->m_pMainWnd = this;


        // run the engine
        if( std::holds_alternative<CRunAplBatch*>(m_batchDriver) )
        {
            CRunAplBatch& batch_application = *std::get<CRunAplBatch*>(m_batchDriver);

            batch_application.Start();
            batch_application.Stop();
            batch_application.End(true);
        }

        else
        {
            BatchDriver& batch_driver = *std::get<BatchDriver*>(m_batchDriver);

            batch_driver.Run();
        }


        // restore the original main window
        AfxGetApp()->m_pMainWnd = saved_main_window;

        // if the user didn't cancel the batch operation, we need to inform BatchMeterDlg that
        // when closing the dialog, it shouldn't prompt about canceling the operation
        SetCompleted();

        // close the dialog
        PostMessage(WM_CLOSE);
    });

    return 0;
}


LRESULT BatchExecutionDlg::OnGetProcessSummaryReporter(WPARAM wParam, LPARAM /*lParam*/)
{
    ProcessSummaryReporter** process_summary_reporter_ptr = reinterpret_cast<ProcessSummaryReporter**>(wParam);
    *process_summary_reporter_ptr = this;

    return 0;
}


LRESULT BatchExecutionDlg::OnEngineAbort(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    AfxMessageBox(_T("Fatal error in engine. CSBatch closing down."));

    ViewFileInTextViewer(m_pff.GetApplicationErrorsFilename());
    Listing::Lister::View(m_pff.GetListingFName());

    exit(0);
}


LRESULT BatchExecutionDlg::OnEngineUI(WPARAM wParam, LPARAM lParam)
{
    return m_engineUIProcessor.ProcessMessage(wParam, lParam);
}


LRESULT BatchExecutionDlg::OnRunOnUIThread(WPARAM wParam, LPARAM /*lParam*/)
{
    UIThreadRunner* ui_thread_runner = reinterpret_cast<UIThreadRunner*>(wParam);
    ui_thread_runner->Execute();
    return 1;
}


LRESULT BatchExecutionDlg::OnGetApplicationShutdownRunner(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return reinterpret_cast<LRESULT>(&m_applicationShutdownRunner);
}
