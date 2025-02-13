#pragma once

#ifdef WIN_DESKTOP

#include <zUtilF/zUtilF.h>
#include <thread>


// a simple modal dialog for displaying a progress bar (with a range of 0-100)

class CLASS_DECL_ZUTILF ThreadedProgressDlg
{
public:
    ThreadedProgressDlg();
    ~ThreadedProgressDlg();

    void Show();

    void SetTitle(const CString& title);
    void SetStatus(const CString& status);

    void SetPos(int position);

    bool IsCanceled() const { return m_canceled; }

private:
    void Close();

    std::unique_ptr<std::thread> m_dialogThread;
    HWND m_hwndDlg;
    CString m_title;
    CString m_status;
    int m_position;
    bool m_canceled;

    static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


#else

// the progress dialog is stubbed out on Android/console

class ThreadedProgressDlg
{
public:
    void Show() { }

    void SetTitle(const CString& /*title*/) { }
    void SetStatus(const CString& /*status*/) { }

    void SetPos(int /*position*/) { }

    bool IsCanceled() const { return false; }
};

#endif
