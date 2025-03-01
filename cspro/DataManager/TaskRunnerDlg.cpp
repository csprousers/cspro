#include "StdAfx.h"
#include "TaskRunnerDlg.h"
#include "WindowsMessageCaseConstructionReporter.h"
#include <zUtilO/WindowHelpers.h>


namespace
{
    constexpr LPARAM TaskEvent_StartTask = 1;
    constexpr LPARAM TaskEvent_SetTitle  = 2;
    constexpr LPARAM TaskEvent_Complete  = 3;
}


BEGIN_MESSAGE_MAP(TaskRunnerDlg, ResizableDlg)
    ON_WM_DESTROY()
    ON_MESSAGE(UWM::DataManager::TaskEvent, OnTaskEvent)
    ON_MESSAGE(UWM::DataManager::CaseConstructionReporterMessage, OnCaseConstructionReporterMessage)
END_MESSAGE_MAP()


TaskRunnerDlg::TaskRunnerDlg(std::unique_ptr<Task> task, CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_TASK, pParent),
        m_usingMarquee(false),
        m_task(std::move(task)),
        m_cancelFlag(false),
        m_closeDialogOnSuccess(false)
{
    ASSERT(m_task != nullptr);

    SerializeDialogSize("TaskRunnerDlg");
}


void TaskRunnerDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
    DDX_Control(pDX, IDC_PROGRESS, m_progressCtrl);
}


BOOL TaskRunnerDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    PostMessage(UWM::DataManager::TaskEvent, 0, TaskEvent_StartTask);

    return TRUE;
}


void TaskRunnerDlg::OnDestroy()
{
    if( m_runThread.joinable() )
        m_runThread.join();

    m_task.reset();

    __super::OnDestroy();
}


std::shared_ptr<CaseConstructionReporter> TaskRunnerDlg::GetCaseConstructionReporter(const CaseAccess& case_access)
{
    return std::make_unique<WindowsMessageCaseConstructionReporter>(this, case_access.GetDataDict().GetName());
}


void TaskRunnerDlg::SetTitle(const std::string& title)
{
    WindowsDesktopMessage::PostObject(this, UWM::DataManager::TaskEvent, SharableString(title), TaskEvent_SetTitle);
}


void TaskRunnerDlg::LogText(SharableString text/* = SharableString()*/)
{
    m_loggingListBox.AddText(std::move(text));
}


void TaskRunnerDlg::ToggleProgressBarMarquee()
{
    m_usingMarquee = !m_usingMarquee;

    const LONG_PTR progress_style = GetWindowLongPtr(m_progressCtrl, GWL_STYLE);

    // add or remove the marquee style
    SetWindowLongPtr(m_progressCtrl, GWL_STYLE, m_usingMarquee ? ( progress_style | PBS_MARQUEE ) :
                                                                 ( progress_style & ~PBS_MARQUEE ));

    // turn on or off the marquee animation
    m_progressCtrl.PostMessage(PBM_SETMARQUEE, m_usingMarquee ? TRUE : FALSE);
}


void TaskRunnerDlg::UpdateProgress(const int percent)
{
    // handle indefinite progress
    if( percent < 0 )
    {
        if( !m_usingMarquee )
            ToggleProgressBarMarquee();
    }

    // handle definite progress
    else
    {
        ASSERT(percent <= 100);

        if( m_usingMarquee )
            ToggleProgressBarMarquee();

        m_progressCtrl.PostMessage(PBM_SETPOS, percent);
    }
}


LRESULT TaskRunnerDlg::OnTaskEvent(const WPARAM wParam, const LPARAM lParam)
{
    switch( lParam )
    {
        case TaskEvent_StartTask:
            OnTaskEvent_StartTask();
            return 1;

        case TaskEvent_SetTitle:
            OnTaskEvent_SetTitle(WindowsDesktopMessage::GetPostedObject<SharableString>(wParam));
            return 1;

        case TaskEvent_Complete:
            OnTaskEvent_Complete();
            return 1;

        default:
            return ReturnProgrammingError(0);
    }
}


LRESULT TaskRunnerDlg::OnCaseConstructionReporterMessage(const WPARAM wParam, LPARAM /*lParam*/)
{
    const std::string* const message = reinterpret_cast<const std::string*>(wParam);
    ASSERT(message != nullptr);

    LogText();
    LogText(u8"⚠ " + *message);
    LogText();

    return 1;
}


void TaskRunnerDlg::OnTaskEvent_StartTask()
{
    m_task->SetTaskRunner(*this, m_cancelFlag);
    m_runThread = std::thread([&]() { RunTask(); });
}


void TaskRunnerDlg::OnTaskEvent_SetTitle(const SharableString text)
{
    WindowsUtf8::SetText(this, IDC_TASK_TITLE, *text);
}


void TaskRunnerDlg::OnTaskEvent_Complete()
{
    ASSERT(m_taskResult.has_value());

    // hide the Cancel button and show the Close button
    GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);

    CWnd* const close_button = GetDlgItem(IDOK);
    close_button->EnableWindow();
    close_button->SetFocus();

    // potentially close the dialog automatically on success
    if( *m_taskResult == Task::Result::Complete )
    {
        if( m_closeDialogOnSuccess )
            PostMessage(WM_COMMAND, IDOK);
    }

    // hide the process bar on error
    else
    {
        m_progressCtrl.ShowWindow(SW_HIDE);
    }
}


void TaskRunnerDlg::RunTask()
{
    Task::Result result;

    try
    {
        m_task->Initialize();

        if( m_cancelFlag )
            throw Task::CanceledException();

        m_task->Run();

        if( m_cancelFlag )
            throw Task::CanceledException();

        UpdateProgress(100);

        result = Task::Result::Complete;
    }

    catch( const Task::CanceledException& )
    {
        result = Task::Result::Canceled;
        LogText("\nThe task was canceled by the user.");
    }

    catch( const std::exception& exception )
    {
        result = Task::Result::Exception;
        LogText(SO::Concatenate(u8"\n⚠ There was an error running the task:\n    ", exception.what()));
    }

    try
    {
        m_task->Finalize(result);
    }

    catch( const std::exception& exception )
    {
        result = Task::Result::Exception;
        LogText(SO::Concatenate(u8"\n⚠ There was an error finalizing the task:\n    ", exception.what()));
    }

    m_taskResult = result;

    PostMessage(UWM::DataManager::TaskEvent, 0, TaskEvent_Complete);
}


void TaskRunnerDlg::OnOK()
{
    ASSERT(m_taskResult.has_value());

    EndDialog(( *m_taskResult == Task::Result::Complete )    ? IDOK :
              ( *m_taskResult == Task::Result::Canceled )    ? IDCANCEL :
            /*( *m_taskResult == Task::Result::Exception )*/   IDABORT);
}


void TaskRunnerDlg::OnCancel()
{
    if( m_taskResult.has_value() )
    {
        // this would occur if the user closes the dialog (e.g., with the Escape key)
        // following the completion of the task
        OnOK();
    }

    else if( m_cancelFlag )
    {
        LogText("The task is in the process of being canceled.");
    }

    else if( MessageBox(L"Are you sure you want to cancel the process?", L"Cancel Process?", MB_YESNO | MB_DEFBUTTON2) == IDYES )
    {
        LogText("Trying to cancel the task...");
        m_cancelFlag = true;
    }
}
