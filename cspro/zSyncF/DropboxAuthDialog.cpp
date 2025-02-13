#include "stdafx.h"
#include "DropboxAuthDialog.h"
#include <zToolsO/Tools.h>
#include <zUtilO/CSProExecutables.h>


BEGIN_MESSAGE_MAP(DropboxAuthDialog, CDialogEx)
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
    ON_MESSAGE(UWM::Sync::DropboxAuthProcessCompleted, OnAuthProcessExited)
END_MESSAGE_MAP()


DropboxAuthDialog::DropboxAuthDialog(CWnd* pParent /*=NULL*/)
    :   CDialogEx(IDD_DROPBOX_AUTH_DIALOG, pParent),
        m_process_wait_handle(nullptr)
{
}


BOOL DropboxAuthDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    if (!RunDropboxAuthProcess()) {
        AfxMessageBox(L"Failed to launch web browser to login to Dropbox");
        EndDialog(IDOK);
    }
    return TRUE;
}


CString DropboxAuthDialog::Show(CString clientId)
{
    if (DoModal() == IDCANCEL) {
        return CString();
    } else {
        return m_auth_token;
    }
}


void DropboxAuthDialog::OnBnClickedCancel()
{
    if (m_process_wait_handle) {
        UnregisterWait(m_process_wait_handle);
        m_process_wait_handle = NULL;
        m_runner.Kill();
    }
    EndDialog(IDCANCEL);
}


LRESULT DropboxAuthDialog::OnAuthProcessExited(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_process_wait_handle) {
        UnregisterWait(m_process_wait_handle);
        m_process_wait_handle = NULL;

        // Bring CSPro back to front (browser is probably in front)
        AfxGetApp()->GetMainWnd()->SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);

        if (m_runner.GetExitCode() == 0) {
            m_auth_token = WS2CS(m_runner.ReadStdOut());
        }
        else {
            AfxMessageBox(FormatText(L"Error connecting to Dropbox: %s", m_runner.ReadStdErr().c_str()));
        }
        EndDialog(IDOK);
    }

    return 0;
}


bool DropboxAuthDialog::RunDropboxAuthProcess()
{
    const std::wstring exe_path = PortableFunctions::PathAppendToPath(CSProExecutables::GetModuleDirectory(), _T("DropboxAuth.exe"));

    std::wstring command_line = EscapeCommandLineArgument(exe_path) + _T(" ") + EscapeCommandLineArgument(CS2WS(GetLocaleLanguage()));

    HANDLE process_handle = m_runner.Start(std::move(command_line));
    if (process_handle == NULL)
        return false;

    RegisterWaitForSingleObject(&m_process_wait_handle, process_handle,
        [](PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/)
        {
            ::PostMessage(reinterpret_cast<HWND>(lpParameter), UWM::Sync::DropboxAuthProcessCompleted, 0, 0);
        },
        reinterpret_cast<PVOID>(GetSafeHwnd()), INFINITE, WT_EXECUTEONLYONCE);

    return true;
}
