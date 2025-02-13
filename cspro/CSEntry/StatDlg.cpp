// StatDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "StatDlg.h"
#include "StatGrid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatDlg dialog


CStatDlg::CStatDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CStatDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CStatDlg)
    m_sVerifiedCases = _T("");
    m_sNumCases = _T("");
    //}}AFX_DATA_INIT
}


void CStatDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStatDlg)
    DDX_Text(pDX, IDC_VERFIEDCASES, m_sVerifiedCases);
    DDX_Text(pDX, IDC_NUMCASES, m_sNumCases);
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// CStatDlg message handlers

BOOL CStatDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //Attach the grid to the control
    m_StatGrid.AttachGrid(this,IDC_STATGRID);

    //Show the grid
    m_StatGrid.ShowWindow(SW_SHOW);
    GetDlgItem(IDOK)->PostMessage(WM_SETFOCUS);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
