#include "StdAfx.h"
#include "ProcessSummaryDlg.h"
#include <zPlatformO/PlatformInterface.h>


#ifdef WIN_DESKTOP

BEGIN_MESSAGE_MAP(ProcessSummaryDlg, BatchMeterDlg)
    ON_MESSAGE(WM_IMSA_START_ENGINE, OnStartTask)
END_MESSAGE_MAP()


ProcessSummaryDlg::ProcessSummaryDlg(CWnd* pParent/* = nullptr*/)
    :   BatchMeterDlg(pParent),
        m_canceled(false)
{
}

ProcessSummaryDlg::~ProcessSummaryDlg()
{
    RunSharedDestructor();
}


BOOL ProcessSummaryDlg::OnInitDialog()
{
    BatchMeterDlg::OnInitDialog();

    PostMessage(WM_IMSA_START_ENGINE);

    return TRUE;
}


void ProcessSummaryDlg::Initialize(const CString& title, std::shared_ptr<ProcessSummary> process_summary)
{
    BatchMeterDlg::Initialize(title, std::move(process_summary), &m_canceled);
}


LRESULT ProcessSummaryDlg::OnStartTask(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_workThread = std::make_unique<std::thread>([&]
    {
        // set the main window to this modal dialog
        CWnd* saved_main_window = AfxGetApp()->m_pMainWnd;
        AfxGetApp()->m_pMainWnd = this;

        // run the task
        RunTask();

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


#else

ProcessSummaryDlg::ProcessSummaryDlg()
    :   m_canceled(false)
{
}


ProcessSummaryDlg::~ProcessSummaryDlg()
{
    RunSharedDestructor();

    if( m_processSummary != nullptr )
        PlatformInterface::GetInstance()->GetApplicationInterface()->HideProgressDialog();
}


void ProcessSummaryDlg::Initialize(const CString& title, std::shared_ptr<ProcessSummary> process_summary, bool* /*cancel_flag*/)
{
    // close the old dialog if one was open
    if( m_processSummary != nullptr )
        PlatformInterface::GetInstance()->GetApplicationInterface()->HideProgressDialog();

    PlatformInterface::GetInstance()->GetApplicationInterface()->ShowProgressDialog(title);

    m_processSummary = std::move(process_summary);

    PlatformInterface::GetInstance()->GetApplicationInterface()->UpdateProgressDialog(m_processSummary->GetPercentSourceRead(), nullptr);
}


void ProcessSummaryDlg::Initialize(const CString& title, std::shared_ptr<ProcessSummary> process_summary)
{
    Initialize(title, std::move(process_summary), &m_canceled);
}


void ProcessSummaryDlg::SetSource(const CString& source_text)
{
    ASSERT(m_processSummary != nullptr);

    m_canceled = PlatformInterface::GetInstance()->GetApplicationInterface()->UpdateProgressDialog(100 * m_processSummary->GetPercentSourceRead(), &source_text);
}


void ProcessSummaryDlg::SetKey(const CString& /*case_key*/)
{
    ASSERT(m_processSummary != nullptr);

    m_canceled = PlatformInterface::GetInstance()->GetApplicationInterface()->UpdateProgressDialog(100 * m_processSummary->GetPercentSourceRead(), nullptr);
}


void ProcessSummaryDlg::DoModal()
{
    RunTask();
}

#endif


void ProcessSummaryDlg::RunSharedDestructor()
{
    if( m_workThread != nullptr && m_workThread->joinable() )
        m_workThread->join();
}


void ProcessSummaryDlg::RunTask()
{
    try
    {
        if( m_task )
            m_task();
    }

    catch(...)
    {
        m_taskException = std::current_exception();
    }
}


void ProcessSummaryDlg::RethrowTaskExceptions()
{
    if( m_taskException )
        std::rethrow_exception(m_taskException);
}
