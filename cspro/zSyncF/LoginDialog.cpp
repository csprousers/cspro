// LoginDialog.cpp : implementation file
//

#include "stdafx.h"
#include "zSyncF.h"
#include "LoginDialog.h"

// CLoginDialog dialog

IMPLEMENT_DYNAMIC(CLoginDialog, CDialogEx)

CLoginDialog::CLoginDialog(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_DIALOG_LOGIN, pParent)
    , m_username(_T(""))
    , m_password(_T(""))
    , m_bShowInvalidPasswordError(false)
{

}

CLoginDialog::~CLoginDialog()
{
}

std::optional<std::tuple<CString, CString>> CLoginDialog::Show(const CString&, bool show_invalid_error)
{
    m_bShowInvalidPasswordError = show_invalid_error;
    if (DoModal() == IDCANCEL) {
        return {};
    } else {
        return std::make_tuple(m_username, m_password);
    }
}

BOOL CLoginDialog::OnInitDialog()
{
    BOOL result = CDialogEx::OnInitDialog();
    if (result) {
        if (!m_bShowInvalidPasswordError) {
            GetDlgItem(IDC_STATIC_ERROR)->ShowWindow(SW_HIDE);
        }
    }
    return result;
}

void CLoginDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_USERNAME, m_username);
    DDX_Text(pDX, IDC_PASSWORD, m_password);
    DDX_Control(pDX, IDC_STATIC_ERROR, m_staticError);
}

void CLoginDialog::OnOK()
{
    UpdateData();
    if (m_username.IsEmpty() || m_password.IsEmpty()) {
        AfxMessageBox(_T("Username and password may not be blank"));
        return;
    }

    CDialogEx::OnOK();
}

HBRUSH CLoginDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if (nCtlColor == CTLCOLOR_STATIC && pWnd == &m_staticError) {
        pDC->SetTextColor(RGB(255, 0, 0));
    }

    return hbr;
}

BEGIN_MESSAGE_MAP(CLoginDialog, CDialogEx)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CLoginDialog message handlers

