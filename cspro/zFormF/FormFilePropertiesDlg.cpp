#include "StdAfx.h"
#include "FormFilePropertiesDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CFFPropDlg dialog


CFFPropDlg::CFFPropDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(CFFPropDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CFFPropDlg)
    m_sFFLabel = _T("");
    m_sFFName = _T("");
    //}}AFX_DATA_INIT
}


void CFFPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFFPropDlg)
    DDX_Text(pDX, IDC_FF_LABEL, m_sFFLabel);
    DDV_MaxChars(pDX, m_sFFLabel, 132);
    DDX_Text(pDX, IDC_FF_NAME, m_sFFName);
    //}}AFX_DATA_MAP
}


CFFPropDlg::CFFPropDlg (CDEFormFile* pFormFile, CFormScrollView* pParent)
    : CDialog(CFFPropDlg::IDD, pParent)
{
    m_sFFName = pFormFile->GetName();
    m_sFFLabel = pFormFile->GetLabel();

    m_pView = pParent;

    m_pFormFile = pFormFile;
}

/////////////////////////////////////////////////////////////////////////////
// CFFPropDlg message handlers

void CFFPropDlg::OnOK()
{
    UpdateData(TRUE);

    if(m_sFFLabel.IsEmpty())
    {
        AfxMessageBox(_T("Label cannot be empty"));
        GetDlgItem (IDC_FF_LABEL)->SetFocus();
    }

    if(m_sFFName.IsEmpty())
    {
        AfxMessageBox(_T("Name cannot be empty"));
        GetDlgItem(IDC_FF_NAME)->SetFocus();
    }

    if(!m_sFFName.IsName())
    {
        AfxMessageBox(_T("Not a Valid Name"));
        return;
    }
    if(m_sFFName.IsReservedWord())
    {
        CIMSAString sMsg;
        sMsg.FormatMessage(_T("%1 is a reserved word"), m_sFFName.GetString());
        AfxMessageBox (sMsg);
        return;
    }
    if((m_pFormFile->GetName().CompareNoCase(m_sFFName) !=0) && m_pView)
    {
        CFormDoc* pDoc  = m_pView->GetDocument();
        if(!AfxGetMainWnd()->SendMessage(UWM::Form::IsNameUnique, (WPARAM)m_sFFName.GetString(), (LPARAM)pDoc))
        {
            GetDlgItem (IDC_FF_NAME)->SetFocus();
            return;
        }
    }
    CDialog::OnOK();
}

BOOL CFFPropDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
