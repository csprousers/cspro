#include "StdAfx.h"
#include "QuestionnaireSearchDlg.h"
#include "CaseView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQuestionnaireSearchDlg dialog


CQuestionnaireSearchDlg::CQuestionnaireSearchDlg(CString csInitialKey,CWnd* pParent /*=NULL*/)
    : CDialog(CQuestionnaireSearchDlg::IDD, pParent)
{
    m_csKey = csInitialKey;
    m_iKeyLength = csInitialKey.GetLength();
}


void CQuestionnaireSearchDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CQuestionnaireSearchDlg)
    DDX_Text(pDX, IDC_KEYEDIT, m_csKey);
    //}}AFX_DATA_MAP
}


void CQuestionnaireSearchDlg::OnOK()
{
    UpdateData(TRUE);

    if( m_csKey.GetLength() != m_iKeyLength )
    {
        CString csErr;
        csErr.Format(_T("Case ID must be %d characters long."),m_iKeyLength);
        AfxMessageBox(csErr);
        return;
    }

    CCaseView* pCaseView = ((CMainFrame*)AfxGetMainWnd())->GetCaseView();
    HTREEITEM hItem = pCaseView->GetTreeCtrl().GetRootItem();
    bool bFound = false;

    while( hItem )
    {
        NODEINFO* pNodeInfo = (NODEINFO*)pCaseView->GetTreeCtrl().GetItemData(hItem);

        if( pNodeInfo->case_summary.GetKey().CompareNoCase(m_csKey) == 0 )
        {
            pCaseView->GetTreeCtrl().Select(hItem,TVGN_CARET);
            bFound = true;
            break;
        }

        hItem = pCaseView->GetTreeCtrl().GetNextSiblingItem(hItem);
    }

    if( !bFound )
    {
        AfxMessageBox(_T("Case not found") );
        return;
    }

    CDialog::OnOK();
}
