#include "StdAfx.h"
#include "ThreadRunnerFrame.h"


IMPLEMENT_DYNCREATE(ThreadRunnerFrame, CMDIChildWnd)


BEGIN_MESSAGE_MAP(ThreadRunnerFrame, CMDIChildWnd)
    ON_WM_CLOSE()
    ON_MESSAGE(UWM::Stygitan::ThreadStart, OnThreadStart)
    ON_MESSAGE(UWM::Stygitan::ThreadIsActive, OnThreadIsActive)
    ON_MESSAGE(UWM::Stygitan::ThreadPromptCancel, OnThreadPromptCancel)
    ON_MESSAGE(UWM::Stygitan::ThreadComplete, OnThreadComplete)
END_MESSAGE_MAP()


ThreadRunnerFrame::ThreadRunnerFrame()
    :   m_threadId(0),
        m_cancelFlag(false)
{
}


void ThreadRunnerFrame::OnClose()
{
    if( m_thread != nullptr && OnThreadPromptCancel(0, 0) == 0 )
        return;

    __super::OnClose();
}


LRESULT ThreadRunnerFrame::OnThreadStart(const WPARAM wParam, const LPARAM lParam)
{
    if( m_thread != nullptr )
        return ReturnProgrammingError(0);

    std::function<void(bool& cancel_flag)>* const thread_function = reinterpret_cast<std::function<void(bool&)>*>(wParam);
    ASSERT(thread_function != nullptr && *thread_function);

    m_threadId = lParam;
    SendMessageToDescendants(UWM::Stygitan::ThreadStart, 0, m_threadId, FALSE);

    m_cancelFlag = false;

    m_thread = std::make_unique<std::thread>(
        [this, thread_function_ = std::move(*thread_function)]()
        {
            thread_function_(m_cancelFlag);
            PostMessage(UWM::Stygitan::ThreadComplete);
        });

    return 1;
}


LRESULT ThreadRunnerFrame::OnThreadIsActive(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return ( m_thread != nullptr ) ? 1 : 0;
}


LRESULT ThreadRunnerFrame::OnThreadPromptCancel(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_thread != nullptr )
    {
        if( m_thread->joinable() )
        {
            if( MessageBox(L"Are you sure you want to cancel the process?",
                           L"Cancel Process?", MB_YESNO | MB_DEFBUTTON2) != IDYES )
            {
                return 0;
            }

            m_cancelFlag = true;
            m_thread->join();
        }

        m_thread.reset();
    }

    return 1;
}


LRESULT ThreadRunnerFrame::OnThreadComplete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_thread != nullptr )
    {
        if( m_thread->joinable() )
            m_thread->join();

        m_thread.reset();
    }

    SendMessageToDescendants(UWM::Stygitan::ThreadComplete, 0, m_threadId, FALSE);

    return 1;
}
