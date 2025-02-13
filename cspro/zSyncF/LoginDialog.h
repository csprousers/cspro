#pragma once

#include <zSyncF/zSyncF.h>
#include <zSyncO/ILoginDialog.h>
#include "afxwin.h"

// CLoginDialog dialog

class ZSYNCF_API CLoginDialog : public CDialogEx, public ILoginDialog
{
    DECLARE_DYNAMIC(CLoginDialog)

public:
    CLoginDialog(CWnd* pParent = NULL);   // standard constructor
    virtual ~CLoginDialog();
    virtual BOOL OnInitDialog();

    std::optional<std::tuple<CString, CString>> Show(const CString& server, bool show_invalid_error) override;

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG_LOGIN };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void OnOK();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    DECLARE_MESSAGE_MAP()

public:
    CString m_username;
    CString m_password;
    bool m_bShowInvalidPasswordError;

private:
    CStatic m_staticError;

};
