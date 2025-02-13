#include "StdAfx.h"
#include "GridPropertiesDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CGridProp dialog


CGridProp::CGridProp(CWnd* pParent /*=NULL*/)
    :   CDialog(CGridProp::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGridProp)
    m_sGridName = _T("");
    m_sGridLabel = _T("");
    m_iHorz = -1;
    m_iFreemovement = -1;
    m_bFreemovement = FALSE;
    m_sMaxOccField = _T("");
    //}}AFX_DATA_INIT

    m_pRoster = NULL;
    m_pMyParent = NULL;

    m_HorzWnd.ShowWindow(SW_HIDE);
    m_VertWnd.ShowWindow(SW_HIDE);
}

CGridProp::CGridProp(CDERoster* pRoster, CFormScrollView* pParent)
    : CDialog(CGridProp::IDD, pParent)
{
    m_pMyParent = pParent;
    m_sGridName = pRoster->GetName();
    m_pRoster = pRoster;
    m_sGridLabel = pRoster->GetLabel();
    m_iHorz = ( pRoster->GetOrientation() == RosterOrientation::Horizontal ) ? 0 : 1;
}

// ************************************************************

void CGridProp::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGridProp)
    DDX_Text(pDX, IDC_GRID_NAME, m_sGridName);
    DDX_Text(pDX, IDC_GRID_LABEL, m_sGridLabel);
    DDX_Radio(pDX, IDC_HORZ, m_iHorz);
    DDX_Control(pDX, IDC_GRID_HORZ, m_HorzWnd);
    DDX_Control(pDX, IDC_GRID_VERT, m_VertWnd);
    DDX_Radio(pDX, IDC_HORZM, m_iFreemovement);
    DDX_Check(pDX, IDC_FREEMOVE, m_bFreemovement);
    DDX_Text(pDX, IDC_MAXOCCFLD, m_sMaxOccField);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGridProp, CDialog)
    //{{AFX_MSG_MAP(CGridProp)
    ON_BN_CLICKED(IDC_HORZ, OnHorz)
    ON_BN_CLICKED(IDC_VERT, OnVert)
    ON_BN_CLICKED(IDC_FREEMOVE, OnFreemove)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGridProp message handlers

BOOL CGridProp::OnInitDialog()
{
    CDialog::OnInitDialog();

    bool horizontal = ( m_pRoster->GetOrientation() == RosterOrientation::Horizontal );
    m_HorzWnd.ShowWindow(horizontal ? SW_SHOW : SW_HIDE);
    m_VertWnd.ShowWindow(horizontal ? SW_HIDE : SW_SHOW);

    if( m_pRoster->UsingFreeMovement() ) {
        m_bFreemovement = TRUE;
        m_iFreemovement = ( m_pRoster->GetFreeMovement() == FreeMovement::Horizontal ) ? 0 : 1;
    }
    else {
        m_bFreemovement = FALSE;
        GetDlgItem(IDC_VERTM)->EnableWindow(FALSE);
        GetDlgItem(IDC_HORZM)->EnableWindow(FALSE);
    }

    // TODO: Add extra initialization here
    m_sMaxOccField = m_pRoster->GetMaxField();
    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CGridProp::OnHorz()
{
    m_HorzWnd.ShowWindow(SW_SHOW);
    m_VertWnd.ShowWindow(SW_HIDE);
}

void CGridProp::OnVert()
{
    m_HorzWnd.ShowWindow(SW_HIDE);
    m_VertWnd.ShowWindow(SW_SHOW);
}

void CGridProp::OnOK()
{
    UpdateData (true);
    m_sMaxOccField.Trim();
    m_sMaxOccField.MakeUpper();

    if (m_sGridLabel.IsEmpty())
    {
        AfxMessageBox(_T("Label cannot be empty"));
        GetDlgItem(IDC_GRID_LABEL)->SetFocus();
    }

    if (m_sGridName.IsEmpty())
    {
        AfxMessageBox(_T("Name cannot be empty"));
        GetDlgItem(IDC_GRID_NAME)->SetFocus();
    }

    if (!m_sGridName.IsName()) {
            AfxMessageBox(_T("Not a Valid Name"));
            return;
    }
    if (m_sGridName.IsReservedWord()) {
            CString sMsg;
            sMsg.FormatMessage(_T("%1 is a reserved word"), m_sGridName.GetString());
            AfxMessageBox(sMsg);
            return;
    }

    if ((m_pRoster->GetName().CompareNoCase(m_sGridName) !=0) && m_pMyParent)
    {
        CFormDoc* pDoc  = m_pMyParent->GetDocument();

        if(!AfxGetMainWnd()->SendMessage(UWM::Form::IsNameUnique, (WPARAM)m_sGridName.GetString(), (LPARAM)pDoc))
        {
            GetDlgItem(IDC_GRID_NAME)->SetFocus();
            return;
        }
    }
    if( !m_sMaxOccField.IsEmpty()){
        CFormDoc* pDoc = m_pMyParent->GetDocument();
        const CDictItem* pItem = pDoc->GetSharedDictionary()->LookupName<CDictItem>(m_sMaxOccField);
        if(pItem == nullptr || pItem->GetContentType() != ContentType::Numeric) {
            AfxMessageBox(FormatText(_T("%s is not a numeric dictionary item name"), m_sMaxOccField.GetString()));
            return;
        }
        else {
            m_pRoster->SetMaxField(m_sMaxOccField);
            pDoc->SetModifiedFlag(TRUE);
        }
    }
    else {
        m_pRoster->SetMaxField(m_sMaxOccField);
        CFormDoc* pDoc = m_pMyParent->GetDocument();
        pDoc->SetModifiedFlag(TRUE);
    }
    CDialog::OnOK();
}

void CGridProp::OnFreemove()
{
    //if the button is off update the  controls
    UpdateData(TRUE);
    if(m_bFreemovement) {
        GetDlgItem(IDC_VERTM)->EnableWindow(TRUE);
        GetDlgItem(IDC_HORZM)->EnableWindow(TRUE);
        m_iFreemovement = 0;
        UpdateData(FALSE);
    }
    else {
        GetDlgItem(IDC_VERTM)->EnableWindow(FALSE);
        GetDlgItem(IDC_HORZM)->EnableWindow(FALSE);
    }
    //if the button is on activate the controls
}


RosterOrientation CGridProp::GetRosterOrientation() const
{
    return ( m_iHorz == 0 ) ? RosterOrientation::Horizontal :
                              RosterOrientation::Vertical;
}


FreeMovement CGridProp::GetFreeMovement() const
{
    return ( m_bFreemovement == FALSE ) ? FreeMovement::Disabled :
           ( m_iFreemovement == 0 )     ? FreeMovement::Horizontal :
                                          FreeMovement::Vertical;
}
