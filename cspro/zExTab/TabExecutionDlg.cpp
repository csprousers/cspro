#include "StdAfx.h"
#include "TabExecutionDlg.h"
#include <zToolsO/UWM.h>
#include <zUtilF/UIThreadRunner.h>
#include <zListingO/Lister.h>
#include <engine/EngineObjectTransporter.h>
#include <zBatchO/Runaplb.h>


BEGIN_MESSAGE_MAP(TabExecutionDlg, BatchMeterDlg)
    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)
    ON_MESSAGE(WM_IMSA_START_ENGINE, OnStartEngine)
    ON_MESSAGE(WM_IMSA_GET_PROCESS_SUMMARY_REPORTER, OnGetProcessSummaryReporter)
    ON_MESSAGE(WM_IMSA_ENGINEABORT, OnEngineAbort)
    ON_MESSAGE(WM_IMSA_PORTABLE_ENGINEUI, OnEngineUI)
    ON_MESSAGE(UWM::UtilF::RunOnUIThread, OnRunOnUIThread)
    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)
END_MESSAGE_MAP()


TabExecutionDlg::TabExecutionDlg(const CNPifFile& pff, CRunAplBatch* run_apl_batch, const std::function<void()>& tab_function, CWnd* pParent/* = nullptr*/)
    :   BatchMeterDlg(pParent),
        m_pff(pff),
        m_runAplBatch(run_apl_batch),
        m_tabFunction(tab_function),
        m_engineUIProcessor(&m_pff, false)
{
}


TabExecutionDlg::~TabExecutionDlg()
{
    if( m_engineThread != nullptr && m_engineThread->joinable() )
        m_engineThread->join();
}


BOOL TabExecutionDlg::OnInitDialog()
{
    BatchMeterDlg::OnInitDialog();

    PostMessage(WM_IMSA_START_ENGINE);

    return TRUE;
}


LRESULT TabExecutionDlg::OnGetObjectTransporter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_objectTransporter == nullptr )
    {
        const CEngineArea* engine_area = ( m_runAplBatch != nullptr ) ? m_runAplBatch->GetEngineArea() :
                                                                        nullptr;

        m_objectTransporter = std::make_unique<EngineObjectTransporter>(engine_area);
    }

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


LRESULT TabExecutionDlg::OnStartEngine(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_engineThread = std::make_unique<std::thread>([&]
    {
        // set the main window to this modal dialog
        CWnd* saved_main_window = AfxGetApp()->m_pMainWnd;
        AfxGetApp()->m_pMainWnd = this;


        // run the tab function
        m_tabFunction();


        // restore the original main window
        AfxGetApp()->m_pMainWnd = saved_main_window;

        // if the user didn't cancel the operation, we need to inform BatchMeterDlg that
        // when closing the dialog, it shouldn't prompt about canceling the operation
        SetCompleted();

        // close the dialog
        PostMessage(WM_CLOSE);
    });

    return 0;
}


LRESULT TabExecutionDlg::OnGetProcessSummaryReporter(WPARAM wParam, LPARAM /*lParam*/)
{
    ProcessSummaryReporter** process_summary_reporter_ptr = reinterpret_cast<ProcessSummaryReporter**>(wParam);
    *process_summary_reporter_ptr = this;

    return 0;
}


LRESULT TabExecutionDlg::OnEngineAbort(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    AfxMessageBox(_T("Fatal error in engine. CSBatch closing down."));

    ViewFileInTextViewer(m_pff.GetApplicationErrorsFilename());
    Listing::Lister::View(m_pff.GetListingFName());

    exit(0);
}


LRESULT TabExecutionDlg::OnEngineUI(WPARAM wParam, LPARAM lParam)
{
    return m_engineUIProcessor.ProcessMessage(wParam, lParam);
}


LRESULT TabExecutionDlg::OnRunOnUIThread(WPARAM wParam, LPARAM /*lParam*/)
{
    UIThreadRunner* ui_thread_runner = reinterpret_cast<UIThreadRunner*>(wParam);
    ui_thread_runner->Execute();
    return 1;
}


LRESULT TabExecutionDlg::OnGetApplicationShutdownRunner(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return reinterpret_cast<LRESULT>(&m_applicationShutdownRunner);
}
