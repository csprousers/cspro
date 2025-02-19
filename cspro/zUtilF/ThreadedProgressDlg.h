#pragma once

#ifdef WIN_DESKTOP

#include <zUtilF/zUtilF.h>
#include <thread>


// --------------------------------------------------------------------------
// ThreadedProgressDlg
//
// A simple modal dialog for displaying a progress bar. The progress bar
// has a range of 0-100, but if SetPos is called with a negative value,
// the progress bar will be modified to be a marquee to indicate indefinite
// progress. If using marquee style, make sure that the executable calls
// InitializeCommonControls in its CWinApp::InitInstance override.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILF ThreadedProgressDlg
{
public:
    ThreadedProgressDlg();
    ~ThreadedProgressDlg();

    // Setting the marquee style prior to showing the dialog will ensure that
    // non-marquee dialog elements don't show when the dialog is first shown.
    void UseMarqueeStyle() { SetPos(-1); }

    void Show();

    void SetTitle(InterfaceString title);
    void SetStatus(InterfaceString status);

    void SetPos(int position);

    bool IsCanceled() const { return m_canceled; }

private:
    static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void Close();

    void ToggleProgressBarMarquee();

private:
    std::unique_ptr<std::thread> m_dialogThread;
    HWND m_hwndDlg;
    std::wstring m_title;
    std::wstring m_status;
    bool m_usingMarquee;
    int m_position;
    bool m_canceled;
};


#else

// the progress dialog is stubbed out on Android/console

class ThreadedProgressDlg
{
public:
    void Show() { }

    void SetTitle(InterfaceString /*title*/) { }
    void SetStatus(InterfaceString /*status*/) { }

    void SetPos(int /*position*/) { }

    bool IsCanceled() const { return false; }
};

#endif
