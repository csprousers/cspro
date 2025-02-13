#include "StdAfx.h"
#include "LevelPropertiesDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CLevPropDlg dialog


CLevPropDlg::CLevPropDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CLevPropDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CLevPropDlg)
    m_sLevelLabel = _T("");
    m_sLevelName = _T("");
    //}}AFX_DATA_INIT
}

CLevPropDlg::CLevPropDlg (CDELevel* pLevel, CFormScrollView* pParent)
    : CDialog(CLevPropDlg::IDD, pParent)
{
    m_pLevel = pLevel;
    m_pView = pParent;
    m_sLevelLabel = pLevel->GetLabel();
    m_sLevelName =  pLevel->GetName();
}
void CLevPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CLevPropDlg)
    DDX_Text(pDX, IDC_LEVEL_LABEL, m_sLevelLabel);
    DDX_Text(pDX, IDC_LEVEL_NAME, m_sLevelName);
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// CLevPropDlg message handlers

void CLevPropDlg::OnOK()
{
    UpdateData(TRUE);
    if(m_sLevelLabel.IsEmpty()){
        AfxMessageBox(_T("Label cannot be empty"));
        GetDlgItem(IDC_LEVEL_LABEL)->SetFocus();
    }

    if(m_sLevelName.IsEmpty()) {
        AfxMessageBox(_T("Name cannot be empty"));
        GetDlgItem(IDC_LEVEL_NAME)->SetFocus();
    }

    if(!m_sLevelName.IsName()) {
            AfxMessageBox(_T("Not a Valid Name"));
            return;
    }
    if(m_sLevelName.IsReservedWord()) {
            CIMSAString sMsg;
            sMsg.FormatMessage(_T("%1 is a reserved word"), m_sLevelName.GetString());
            AfxMessageBox(sMsg);
            return;
    }

    if((m_pLevel->GetName().CompareNoCase(m_sLevelName) !=0) && m_pView) {
        CFormDoc* pDoc  = m_pView->GetDocument();
        if(!AfxGetMainWnd()->SendMessage(UWM::Form::IsNameUnique, (WPARAM)m_sLevelName.GetString(), (LPARAM)pDoc)) {
            GetDlgItem(IDC_LEVEL_NAME)->SetFocus();
            return;
        }
    }
    CDialog::OnOK();
}

BOOL CLevPropDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CWnd* pWnd = GetDlgItem(IDC_LEVEL_NAME);
    pWnd->EnableWindow(FALSE); // Do not let the user change the level name 06/27/01

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
