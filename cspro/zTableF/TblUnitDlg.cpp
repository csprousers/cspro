// TblUtDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "TblUtDlg.h"


// CTblUnitDlg dialog

IMPLEMENT_DYNAMIC(CTblUnitDlg, CDialog)
CTblUnitDlg::CTblUnitDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CTblUnitDlg::IDD, pParent)
    , m_sUnitName(_T(""))
{
}

CTblUnitDlg::~CTblUnitDlg()
{
}

void CTblUnitDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_UNIT_SUBTABLES, m_cUnitNames);
    DDX_CBString(pDX, IDC_UNIT_SUBTABLES, m_sUnitName);
}


BEGIN_MESSAGE_MAP(CTblUnitDlg, CDialog)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CTblUnitDlg message handlers

void CTblUnitDlg::OnOK()
{
    // TODO: Add your specialized code here and/or call the base class

    CDialog::OnOK();
}

void CTblUnitDlg::OnBnClickedOk()
{
    UpdateData(TRUE);
    if(m_sUnitName.CompareNoCase(_T("Select Unit -To Apply To All Subtables")) ==0){
        AfxMessageBox(_T("Please select a unit from the list to apply to all subtables"));
        return;
    }
    OnOK();
}

BOOL CTblUnitDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    for(int iIndex =0; iIndex < m_arrUnitNames.GetSize(); iIndex++){
        m_cUnitNames.AddString(m_arrUnitNames[iIndex]);
    }
    m_cUnitNames.SetCurSel(0);
    // TODO:  Add extra initialization here

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
