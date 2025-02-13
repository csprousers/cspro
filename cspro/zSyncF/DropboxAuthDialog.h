#pragma once

#include <zSyncF/zSyncF.h>
#include <zSyncO/IDropboxAuthDialog.h>
#include <zToolsO/ProcessRunner.h>


// DropboxAuthDialog dialog

class ZSYNCF_API DropboxAuthDialog : public CDialogEx, public IDropboxAuthDialog
{
public:
    DropboxAuthDialog(CWnd* pParent = NULL);   // standard constructor

    CString Show(CString clientId) override;

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    void OnBnClickedCancel();
    LRESULT OnAuthProcessExited(WPARAM wParam, LPARAM lParam);

private:
    bool RunDropboxAuthProcess();

private:
    ProcessRunner m_runner;
    HANDLE m_process_wait_handle;
    CString m_auth_token;
};
