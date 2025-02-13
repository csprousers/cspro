#include "StdAfx.h"
#include "ThreadedProgressDlg.h"
#include <zUtilO/WindowHelpers.h>
#include <mutex>


namespace
{
    const WPARAM UpdateAll = 0;
    const WPARAM UpdateTitle = 1;
    const WPARAM UpdateStatus = 2;
    const WPARAM UpdatePos = 3;

    std::map<HWND, ThreadedProgressDlg*> Instances;
    ThreadedProgressDlg* CurrentInstance = nullptr;
    std::mutex InstancesMutex;
}


ThreadedProgressDlg::ThreadedProgressDlg()
    :   m_hwndDlg(nullptr),
        m_title(_T("CSPro")),
        m_status(_T("CSPro is working...")),
        m_position(0),
        m_canceled(false)
{
}

ThreadedProgressDlg::~ThreadedProgressDlg()
{
    Close();
}


INT_PTR CALLBACK ThreadedProgressDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    auto get_instance = [hwndDlg]() -> ThreadedProgressDlg*
    {
        std::lock_guard<std::mutex> instances_guard(InstancesMutex);
        const auto& instance_search = Instances.find(hwndDlg);
        return ( instance_search != Instances.cend() ) ? instance_search->second : nullptr;
    };

    if( uMsg == WM_INITDIALOG )
    {
        Instances.insert(std::make_pair(hwndDlg, CurrentInstance));
        CurrentInstance->m_hwndDlg = hwndDlg;

        WindowHelpers::DisableClose(hwndDlg);
        WindowHelpers::CenterOnScreen(hwndDlg);

        PostMessage(hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdateAll, 0);

        return TRUE;
    }

    else if( uMsg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDCANCEL )
    {
        ThreadedProgressDlg* instance = get_instance();
        if( instance != nullptr )
            instance->m_canceled = true;
        return TRUE;
    }

    else if( uMsg == WM_CLOSE )
    {
        ThreadedProgressDlg* instance = get_instance();
        EndDialog(hwndDlg, ( instance == nullptr || instance->m_canceled ) ? IDCANCEL : IDOK);
        return TRUE;
    }

    else if( uMsg == UWM::UtilF::UpdateThreadedProgressDlg )
    {
        ThreadedProgressDlg* instance = get_instance();

        if( instance != nullptr )
        {
            if( wParam == UpdateAll || wParam == UpdateTitle )
                SetWindowText(hwndDlg, instance->m_title);

            if( wParam == UpdateAll || wParam == UpdateStatus )
                SetWindowText(GetDlgItem(hwndDlg, IDC_PROGDLG_STATUS), instance->m_status);

            if( wParam == UpdateAll || wParam == UpdatePos )
            {
                CString percent_text;
                percent_text.Format(_T("%d%%"), instance->m_position);
                SetWindowText(GetDlgItem(hwndDlg, IDC_PROGDLG_PERCENT), percent_text);

                PostMessage(GetDlgItem(hwndDlg, IDC_PROGDLG_PROGRESS), PBM_SETPOS, instance->m_position, 0);
            }

            return TRUE;
        }
    }

    return FALSE;
}


void ThreadedProgressDlg::Show()
{
    if( m_dialogThread == nullptr )
    {
        std::lock_guard<std::mutex> instances_guard(InstancesMutex);
        CurrentInstance = this;

        m_dialogThread = std::make_unique<std::thread>([&]
        {
            DialogBox(zUtilFDLL.hModule, MAKEINTRESOURCE(IDD_PROGRESS), nullptr, DialogProc);
        });

        // wait until DialogBox is setup
        while( m_hwndDlg == nullptr )
            Sleep(5);
    }
}

void ThreadedProgressDlg::Close()
{
    if( m_dialogThread != nullptr )
    {
        if( m_hwndDlg != nullptr )
        {
            SendMessage(m_hwndDlg, WM_CLOSE, 0, 0);

            std::lock_guard<std::mutex> instances_guard(InstancesMutex);
            Instances.erase(m_hwndDlg);
            m_hwndDlg = nullptr;
        }

        m_dialogThread->join();
        m_dialogThread.reset();
    }
}


void ThreadedProgressDlg::SetTitle(const CString& title)
{
    m_title = title;

    if( m_hwndDlg != nullptr )
        PostMessage(m_hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdateTitle, 0); 
}

void ThreadedProgressDlg::SetStatus(const CString& status)
{
    m_status = status;

    if( m_hwndDlg != nullptr )
        PostMessage(m_hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdateStatus, 0); 
}

void ThreadedProgressDlg::SetPos(int position)
{
    m_position = position;

    if( m_hwndDlg != nullptr )
        PostMessage(m_hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdatePos, 0); 
}
