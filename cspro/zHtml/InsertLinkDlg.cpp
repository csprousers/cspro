#include "stdafx.h"
#include "InsertLinkDlg.h"
#include <zUtilO/DataExchange.h>


InsertLinkDlg::InsertLinkDlg(CWnd* pParent /*=nullptr*/)
	:   CDialog(IDD_INSERT_LINK, pParent),
        m_url(_T("https://"))
{
}


void InsertLinkDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_LINK_TEXT, m_text);
    DDX_Text(pDX, IDC_LINK_URL, m_url, true);
}


void InsertLinkDlg::OnOK()
{
    UpdateData(TRUE);

    if( m_text.empty() || m_url.empty() )
    {
        AfxMessageBox(_T("Please enter values for the text to display and the URL."));
        return;
    }

    CDialog::OnOK();
}
