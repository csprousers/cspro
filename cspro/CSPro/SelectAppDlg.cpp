#include "StdAfx.h"
#include "SelectAppDlg.h"


SelectAppDlg::SelectAppDlg(std::vector<CAplDoc*> application_docs, const TCHAR* dialog_title/* = nullptr*/, CWnd* pParent/* = nullptr*/)
    :   CDialog(SelectAppDlg::IDD, pParent),
        m_applicationDocs(std::move(application_docs)),
        m_dialogTitle(dialog_title),
        m_selectedIndex(-1)
{
    ASSERT(!m_applicationDocs.empty());
}


void SelectAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_LBIndex(pDX, IDC_APPLIST, m_selectedIndex);
}


BOOL SelectAppDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // use the title if not empty
    if( !m_dialogTitle.IsEmpty() )
        SetWindowText(m_dialogTitle);

    // populate the list and select the first application
    CListBox* pList = (CListBox*)GetDlgItem(IDC_APPLIST);

    for( const CAplDoc* pAplDoc : m_applicationDocs )
        pList->AddString(pAplDoc->GetAppObject().GetLabel());

    m_selectedIndex = 0;

    UpdateData(FALSE);

    return TRUE;
}


CAplDoc* SelectAppDlg::GetSelectedApplicaton() const
{
    return ( m_selectedIndex < 0 ) ? nullptr :
                                     m_applicationDocs[m_selectedIndex];
}
