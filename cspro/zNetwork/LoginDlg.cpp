#include "stdafx.h"
#include "LoginDlg.h"
#include "resource.h"
#include <zNetwork/LoginCredentials.h>
#include <zUtilO/DataExchange.h>


BEGIN_MESSAGE_MAP(LoginDlg, CDialog)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


LoginDlg::LoginDlg(const bool show_invalid_error, CWnd* const pParent /* = nullptr*/)
    :   CDialog(IDD_LOGIN, pParent),
        m_showInvalidPasswordError(show_invalid_error)
{
}


void LoginDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_USERNAME, m_usernamePassword.username);
    DDX_Text(pDX, IDC_PASSWORD, m_usernamePassword.password);
    DDX_Control(pDX, IDC_STATIC_ERROR, m_staticError);
}


BOOL LoginDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    if( result && !m_showInvalidPasswordError )
        GetDlgItem(IDC_STATIC_ERROR)->ShowWindow(SW_HIDE);

    return result;
}


void LoginDlg::OnOK()
{
    UpdateData();

    if( m_usernamePassword.username.empty() || m_usernamePassword.password.empty() )
    {
        AfxMessageBox(L"Username and password cannot be blank.");
        return;
    }

    __super::OnOK();
}


HBRUSH LoginDlg::OnCtlColor(CDC* const pDC, CWnd* const pWnd, const UINT nCtlColor)
{
    HBRUSH hbr = __super::OnCtlColor(pDC, pWnd, nCtlColor);

    if( nCtlColor == CTLCOLOR_STATIC && pWnd == &m_staticError )
        pDC->SetTextColor(RGB(255, 0, 0));

    return hbr;
}
