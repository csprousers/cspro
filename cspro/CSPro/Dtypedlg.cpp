// DTypeDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "Dtypedlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDictTypeDlg dialog


CDictTypeDlg::CDictTypeDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDictTypeDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDictTypeDlg)
    m_iDType = -1;
    //}}AFX_DATA_INIT
    m_bMain = FALSE;
    m_bSpecialOutPut = FALSE;

}


void CDictTypeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDictTypeDlg)
    DDX_Control(pDX, IDC_DICTTYPE, m_DictType);
    DDX_Radio(pDX, IDC_DICTTYPE, m_iDType);
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// CDictTypeDlg message handlers

BOOL CDictTypeDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    this->m_DictType.EnableWindow(FALSE);
    if(m_bMain) {//Every thing is diabled and m_iDType = 0
        m_iDType =0;
        this->GetDlgItem(IDC_RADIOWKG)->EnableWindow(FALSE);
        this->GetDlgItem(IDC_RADIOOUTPUT)->EnableWindow(FALSE);
        this->GetDlgItem(IDC_RADIOEXT)->EnableWindow(FALSE);
    }
    else {
        if(m_bSpecialOutPut){
            this->GetDlgItem(IDC_RADIOOUTPUT)->EnableWindow(TRUE);
        }
        else {
            this->GetDlgItem(IDC_RADIOOUTPUT)->EnableWindow(FALSE);
        }
        this->GetDlgItem(IDC_RADIOWKG)->EnableWindow(TRUE);
        this->GetDlgItem(IDC_RADIOEXT)->EnableWindow(TRUE);
    }
    UpdateData(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDictTypeDlg::OnOK()
{
    CDialog::OnOK();
}
