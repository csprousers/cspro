// PrintDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "PrintDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrintDlg dialog


CPrintDlg::CPrintDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CPrintDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CPrintDlg)
    m_iPrintBrief = -1;
    m_iPrintNameFirst = -1;
    m_iPrintToFile = -1;
    //}}AFX_DATA_INIT
}


void CPrintDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPrintDlg)
    DDX_Radio(pDX, IDC_PRINT_DETAILED, m_iPrintBrief);
    DDX_Radio(pDX, IDC_PRINT_LABEL_FIRST, m_iPrintNameFirst);
    DDX_Radio(pDX, IDC_PRINT_PRINTER, m_iPrintToFile);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrintDlg, CDialog)
    //{{AFX_MSG_MAP(CPrintDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrintDlg message handlers

BOOL CPrintDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    if (m_bPreview) {
        SetWindowText(_T("Print Preview"));
        GetDlgItem(IDC_PRINT_PRINTER)->EnableWindow(FALSE);
        GetDlgItem(IDC_PRINT_FILE)->EnableWindow(FALSE);
    }
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
