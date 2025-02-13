// IntEDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "IntEDlg.h"
#include "MainFrm.h"
#include "Rundoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIntEdtDlg dialog


CIntEdtDlg::CIntEdtDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CIntEdtDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CIntEdtDlg)
    m_iOption = 2;
    //}}AFX_DATA_INIT
}


void CIntEdtDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CIntEdtDlg)
    DDX_Radio(pDX, IDC_MSGRAD, m_iOption);
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CIntEdtDlg message handlers

BOOL CIntEdtDlg::OnInitDialog()
{
    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pFrame->GetActiveDocument();
    switch( pDoc->GetPifFile()->GetInteractiveEditMode() )
    {
    case InteractiveEditMode::ErrMsg:
        m_iOption =0;
        break;
    case InteractiveEditMode::Range:
        m_iOption = 1;
        break;
    case InteractiveEditMode::Both:
        m_iOption = 2;
        break;
    case InteractiveEditMode::Ask:
    case InteractiveEditMode::Off:
        m_iOption = -1;
        break;
    default:
        break;
    }
    CDialog::OnInitDialog();

    // TODO: Add extra initialization here

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CIntEdtDlg::OnOK()
{
    // TODO: Add extra validation here
    CDialog::OnOK();

    CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pFrame->GetActiveDocument();
    CNPifFile* pPifFile =pDoc->GetPifFile();
    CEntryrunApp* pApp = (CEntryrunApp*)AfxGetApp();

    switch (m_iOption) {
    case 0 :
        pPifFile->SetInteractiveEditMode(InteractiveEditMode::ErrMsg);
        break;
    case 1:
        pPifFile->SetInteractiveEditMode(InteractiveEditMode::Range);
        break;
    case 2:
        pPifFile->SetInteractiveEditMode(InteractiveEditMode::Both);
        break;
    default:
        break;
    }
}
