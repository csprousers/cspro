#include "StdAfx.h"
#include "RunAsBatchDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CRunBOpDlg dialog


CRunBOpDlg::CRunBOpDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CRunBOpDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CRunBOpDlg)
    m_bSkipStruc = FALSE;
    m_bCheckRanges = FALSE;
    //}}AFX_DATA_INIT
}


void CRunBOpDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRunBOpDlg)
    DDX_Check(pDX, IDC_SKIPSTRUC, m_bSkipStruc);
    DDX_Check(pDX, IDC_CHECKRANGES, m_bCheckRanges);
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CRunBOpDlg message handlers

BOOL CRunBOpDlg::OnInitDialog()
{
    m_bSkipStruc = AfxGetApp()->GetProfileInt(_T("Settings"), _T("SkipStructure"), 1);
    m_bCheckRanges = AfxGetApp()->GetProfileInt(_T("Settings"), _T("CheckRanges"), 1);

    CDialog::OnInitDialog();

    // TODO: Add extra initialization here

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CRunBOpDlg::OnOK()
{
    // TODO: Add extra validation here
    CDialog::OnOK();
    AfxGetApp()->WriteProfileInt(_T("Settings"), _T("SkipStructure"), m_bSkipStruc);
    AfxGetApp()->WriteProfileInt(_T("Settings"), _T("CheckRanges"), m_bCheckRanges);
}
