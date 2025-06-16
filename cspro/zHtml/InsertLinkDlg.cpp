#include "stdafx.h"
#include "InsertLinkDlg.h"
#include <zUtilO/DataExchange.h>


InsertLinkDlg::InsertLinkDlg(std::string text, std::string url, CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_INSERT_LINK, pParent),
        m_text(std::move(text)),
        m_url(url.empty() ? std::string("https://") : std::move(url))
{
}


void InsertLinkDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_LINK_TEXT, m_text);
    DDX_Text(pDX, IDC_LINK_URL, m_url, true);
}


void InsertLinkDlg::OnOK()
{
    UpdateData(TRUE);

    if( m_text.empty() || m_url.empty() )
    {
        AfxMessageBox(L"Please enter values for the text to display and the URL.");
        return;
    }

    __super::OnOK();
}
