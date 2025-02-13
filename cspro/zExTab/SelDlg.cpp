// SelDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "SelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelDlg dialog


CSelDlg::CSelDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_SELECTDLG, pParent)
{
    //{{AFX_DATA_INIT(CSelDlg)
    m_iProcess = -1;
    //}}AFX_DATA_INIT
    m_bDisableCon = true;
    m_bHideAll = true;
}


void CSelDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSelDlg)
    DDX_Radio(pDX, IDC_ALL, m_iProcess);
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CSelDlg message handlers

void CSelDlg::OnOK()
{
    UpdateData(TRUE);
    CDialog::OnOK();

    if(((CButton*)GetDlgItem(IDC_CSFORMAT))->GetCheck()){
        m_iProcess = CS_PREP;
    }
}

BOOL CSelDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    if(m_bDisableCon){
        GetDlgItem(IDC_CSCON)->EnableWindow(FALSE);
    }
    else {
        GetDlgItem(IDC_CSCON)->EnableWindow(TRUE);
    }
    if(m_bHideAll){
        GetDlgItem(IDC_ALL)->ShowWindow(SW_HIDE);
    }
    else {
        GetDlgItem(IDC_ALL)->ShowWindow(SW_SHOW);
    }
    // TODO:  Add extra initialization here

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
