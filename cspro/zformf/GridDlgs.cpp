#include "StdAfx.h"
#include "GridDlgs.h"


/////////////////////////////////////////////////////////////////////////////
// CGridBoxDlg dialog

CGridBoxDlg::CGridBoxDlg(RosterOrientation roster_orientation, bool box_in_every_cell, CWnd* pParent/* = nullptr*/)
    :   CDialog(CGridBoxDlg::IDD, pParent),
        m_orientationText(( roster_orientation == RosterOrientation::Horizontal ) ? _T("column") : _T("row")),
        m_boxInEveryCell(box_in_every_cell ? 0 : 1)
{
    m_dlgMessage.Format(IDS_GRIDTEXTMSG1, _T("box"), m_orientationText);
}


void CGridBoxDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_GRIDBOXRADIO1, m_boxInEveryCell);
    DDX_Text(pDX, IDC_GRIDBOXMSG1, m_dlgMessage);
}


BOOL CGridBoxDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CString text;
    text.Format(IDS_GRIDTEXTMSG2, _T("box"), m_orientationText);
    GetDlgItem(IDC_GRIDBOXRADIO1)->SetWindowText(text);

    text.Format(IDS_GRIDTEXTMSG3, _T("box"));
    GetDlgItem(IDC_GRIDBOXRADIO2)->SetWindowText(text);

    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CGridTextDlg dialog

CGridTextDlg::CGridTextDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(CGridTextDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGridTextDlg)
    m_csMsg1 = _T("");
    m_csTxt = _T("");
    m_iFont = -1;
    m_iMulti = -1;
    //}}AFX_DATA_INIT
//    m_bHideRadioButtons=m_bFontChanged=false;
    m_bDisableTxt=false;
    m_applytoall = false;
}


void CGridTextDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGridTextDlg)
    DDX_Text(pDX, IDC_GRIDTEXTMSG1, m_csMsg1);
    DDX_Text(pDX, IDC_TXT, m_csTxt);
    DDX_Radio(pDX, IDC_RADIOFONT, m_iFont);
    DDX_Radio(pDX, IDC_GRIDTEXTRADIO1, m_iMulti);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGridTextDlg, CDialog)
    //{{AFX_MSG_MAP(CGridTextDlg)
    ON_BN_CLICKED(IDC_FONT, OnFont)
    ON_BN_CLICKED(IDC_RADIOFONT, OnRadiofont)
    ON_BN_CLICKED(IDC_RADIOFONT2, OnRadiofont2)
    ON_BN_CLICKED(IDC_COLOR, OnColor)
    ON_BN_CLICKED(IDC_APPLY, OnApply)
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_RESET, OnReset)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGridTextDlg message handlers

BOOL CGridTextDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CString cs;
    cs.Format(IDS_DEFAULTFONT, PortableFont(m_lfDefault).GetDescription().GetString());
    GetDlgItem(IDC_RADIOFONT)->SetWindowText(cs);

    GetDlgItem(IDC_FONT)->EnableWindow(m_iFont==1?true:false);
    GetDlgItem(IDC_TEXTSTATIC)->EnableWindow(!m_bDisableTxt);
    GetDlgItem(IDC_TXT)->EnableWindow(!m_bDisableTxt);
    GetDlgItem(IDC_GRIDTEXTRADIO1)->SetWindowText(m_csRadio1);
    GetDlgItem(IDC_GRIDTEXTRADIO2)->SetWindowText(m_csRadio2);

    return TRUE;
}


void CGridTextDlg::OnFont()
{
    LOGFONT lfFont;
    LOGFONT lfNull;

    UpdateData();
    ASSERT(m_iFont==1);
    ASSERT(m_lfDefault.lfHeight!=0);
    memset((void*)&lfNull, 0, sizeof(LOGFONT));
    if (memcmp((void*)&m_lfCustom, (void*)&lfNull, sizeof(LOGFONT))==0)  {
        // no custom font info available, start with default
        lfFont = m_lfDefault;
    }
    else  {
        // use custom font info
        lfFont = m_lfCustom;
    }

//    CFontDialog dlg(&lfFont);     // Change to this to add color  BMD 2 Apr 2001
    CTextFontDialog dlg(&lfFont);//, CF_SCREENFONTS
    //dlg.m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
    dlg.m_cf.rgbColors = m_color;

    if (lfFont.lfFaceName[0]=='\0')  {
        dlg.m_cf.Flags |= CF_NOFACESEL;
    }
    if (lfFont.lfHeight==0)  {
        dlg.m_cf.Flags |= CF_NOSIZESEL;
    }
    if (lfFont.lfWeight==0)  {
        dlg.m_cf.Flags |= CF_NOSTYLESEL;
    }
    if (dlg.DoModal()==IDOK)  {
        LOGFONT lfSelected = *dlg.m_cf.lpLogFont;
        m_color = dlg.GetColor();
        if(memcmp((void*)&m_lfCustom,(void*)&lfSelected,sizeof(LOGFONT)) !=0 ) {
            m_lfCustom = *dlg.m_cf.lpLogFont;
        }
    }
    GetDlgItem(IDOK)->SetFocus();
}

void CGridTextDlg::OnRadiofont()
{
    GetDlgItem(IDC_FONT)->EnableWindow(FALSE);
}

void CGridTextDlg::OnRadiofont2()
{
    GetDlgItem(IDC_FONT)->EnableWindow();
}

void CGridTextDlg::OnColor()
{
    CColorDialog dlg;
    dlg.m_cc.rgbResult = m_color;
    dlg.m_cc.Flags |=  CC_RGBINIT|CC_SOLIDCOLOR;
    if(dlg.DoModal()==IDOK)
    {
        m_color = dlg.GetColor();
        GetDlgItem(IDC_COLOR)->Invalidate();
    }
}

void CGridTextDlg::OnApply()
{
    if (AfxMessageBox(_T("The color will be applied to all text on all forms.\nDo you want to continue?"),MB_YESNO|MB_ICONEXCLAMATION, 0) == IDYES) {
        m_applytoall = true;
    }
}

void CGridTextDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if  (nIDCtl == IDC_COLOR)
    {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);
        dc.FillSolidRect( &lpDrawItemStruct->rcItem,m_color);//COLORREF
            dc.DrawEdge(&lpDrawItemStruct->rcItem, BDR_RAISEDOUTER ,BF_RECT);
    }
    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CGridTextDlg::OnReset()
{
    m_color = RGB(0,0,0);
    UpdateData(TRUE);
    GetDlgItem(IDC_COLOR)->Invalidate();
}



/////////////////////////////////////////////////////////////////////////////
// CJoinDlg dialog

CJoinDlg::CJoinDlg(RosterOrientation roster_orientation, const CString& suggested_label, CWnd* pParent/* = nullptr*/)
    :   CDialog(CJoinDlg::IDD, pParent),
        m_label(suggested_label)
{
    m_dlgMessage.Format(_T("Enter text for the joined %s:"), ( roster_orientation == RosterOrientation::Horizontal ) ? _T("columns") : _T("rows"));
}


void CJoinDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_JOIN_MSG, m_dlgMessage);
    DDX_Text(pDX, IDC_JOIN_TEXT, m_label);
}
