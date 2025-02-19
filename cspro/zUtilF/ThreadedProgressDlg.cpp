#include "StdAfx.h"
#include "ThreadedProgressDlg.h"
#include <zUtilO/WindowHelpers.h>
#include <mutex>


namespace
{
    constexpr WPARAM UpdateAll = 0;
    constexpr WPARAM UpdateTitle = 1;
    constexpr WPARAM UpdateStatus = 2;
    constexpr WPARAM UpdatePos = 3;

    std::map<HWND, ThreadedProgressDlg*> Instances;
    ThreadedProgressDlg* CurrentInstance = nullptr;
    std::mutex InstancesMutex;
}


ThreadedProgressDlg::ThreadedProgressDlg()
    :   m_hwndDlg(nullptr),
        m_title(L"CSPro"),
        m_status(L"CSPro is working..."),
        m_usingMarquee(false),
        m_position(0),
        m_canceled(false)
{
}


ThreadedProgressDlg::~ThreadedProgressDlg()
{
    Close();
}


INT_PTR CALLBACK ThreadedProgressDlg::DialogProc(const HWND hwndDlg, const UINT uMsg, const WPARAM wParam, LPARAM /*lParam*/)
{
    auto get_instance = [hwndDlg]() -> ThreadedProgressDlg*
    {
        std::lock_guard<std::mutex> instances_guard(InstancesMutex);
        const auto& instance_search = Instances.find(hwndDlg);
        return ( instance_search != Instances.cend() ) ? instance_search->second : nullptr;
    };

    if( uMsg == WM_INITDIALOG )
    {
        Instances.try_emplace(hwndDlg, CurrentInstance);
        CurrentInstance->m_hwndDlg = hwndDlg;

        WindowHelpers::DisableClose(hwndDlg);
        WindowHelpers::CenterOnScreen(hwndDlg);

        PostMessage(hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdateAll, 0);

        return TRUE;
    }

    else if( uMsg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDCANCEL )
    {
        ThreadedProgressDlg* const instance = get_instance();
        if( instance != nullptr )
            instance->m_canceled = true;
        return TRUE;
    }

    else if( uMsg == WM_CLOSE )
    {
        ThreadedProgressDlg* const instance = get_instance();
        EndDialog(hwndDlg, ( instance == nullptr || instance->m_canceled ) ? IDCANCEL : IDOK);
        return TRUE;
    }

    else if( uMsg == UWM::UtilF::UpdateThreadedProgressDlg )
    {
        ThreadedProgressDlg* const instance = get_instance();

        if( instance != nullptr )
        {
            if( wParam == UpdateAll || wParam == UpdateTitle )
                WindowsWS::SetWindowText(hwndDlg, instance->m_title);

            if( wParam == UpdateAll || wParam == UpdateStatus )
                WindowsWS::SetDlgItemText(hwndDlg, IDC_PROGDLG_STATUS, instance->m_status);

            if( wParam == UpdateAll || wParam == UpdatePos )
            {
                // handle indefinite progress
                if( instance->m_position < 0 )
                {
                    if( !instance->m_usingMarquee )
                        instance->ToggleProgressBarMarquee();
                }

                // handle definite progress
                else
                {
                    ASSERT(instance->m_position <= 100);

                    if( instance->m_usingMarquee )
                        instance->ToggleProgressBarMarquee();

                    const std::wstring percent_text = FormatTextCS2WS(L"%d%%", instance->m_position);
                    WindowsWS::SetDlgItemText(hwndDlg, IDC_PROGDLG_PERCENT, percent_text);

                    PostMessage(GetDlgItem(hwndDlg, IDC_PROGDLG_PROGRESS), PBM_SETPOS, instance->m_position, 0);
                }
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


void ThreadedProgressDlg::ToggleProgressBarMarquee()
{
    m_usingMarquee = !m_usingMarquee;

    // show or hide the % text indicator
    ShowWindow(GetDlgItem(m_hwndDlg, IDC_PROGDLG_PERCENT), m_usingMarquee ? SW_HIDE : SW_SHOW);

    const HWND hwnd_progress = GetDlgItem(m_hwndDlg, IDC_PROGDLG_PROGRESS);
    const LONG_PTR progress_style = GetWindowLongPtr(hwnd_progress, GWL_STYLE);

    // add or remove the marquee style
    SetWindowLongPtr(hwnd_progress, GWL_STYLE, m_usingMarquee ? ( progress_style | PBS_MARQUEE ) :
                                                                ( progress_style & ~PBS_MARQUEE ));

    // turn on or off the marquee animation
    PostMessage(hwnd_progress, PBM_SETMARQUEE, m_usingMarquee ? TRUE : FALSE, 0);
}


void ThreadedProgressDlg::SetTitle(InterfaceString title)
{
    m_title = title.Release();

    if( m_hwndDlg != nullptr )
        PostMessage(m_hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdateTitle, 0);
}


void ThreadedProgressDlg::SetStatus(InterfaceString status)
{
    m_status = status.Release();

    if( m_hwndDlg != nullptr )
        PostMessage(m_hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdateStatus, 0);
}


void ThreadedProgressDlg::SetPos(const int position)
{
    m_position = position;

    if( m_hwndDlg != nullptr )
        PostMessage(m_hwndDlg, UWM::UtilF::UpdateThreadedProgressDlg, UpdatePos, 0);
}
