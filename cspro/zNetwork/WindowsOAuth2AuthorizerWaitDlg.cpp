#include "stdafx.h"
#include "WindowsOAuth2AuthorizerWaitDlg.h"
#include "resource.h"
#include <zUtilO/WindowsUtf8.h>


BEGIN_MESSAGE_MAP(WindowsOAuth2AuthorizerWaitDlg, CDialog)
    ON_BN_CLICKED(IDCANCEL, OnCancelClick)
END_MESSAGE_MAP()


WindowsOAuth2AuthorizerWaitDlg::WindowsOAuth2AuthorizerWaitDlg(std::string client_name, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_OAUTH_WAIT, pParent),
        m_clientName(std::move(client_name))
{
}


BOOL WindowsOAuth2AuthorizerWaitDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // show the client name in the message
    CWnd* const message_wnd = GetDlgItem(IDC_OAUTH_WAIT_MESSAGE);
    std::string message = WindowsUtf8::GetText(message_wnd);
    SO::Replace(message, "...", m_clientName);
    WindowsUtf8::SetText(message_wnd, message);

    return TRUE;
}


void WindowsOAuth2AuthorizerWaitDlg::OnCancelClick()
{
    EndDialog(IDCANCEL);
}
