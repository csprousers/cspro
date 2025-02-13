#include "StdAfx.h"
#include "ColumnPropertiesDlg.h"


BEGIN_MESSAGE_MAP(CColumnProp, CDialog)
    ON_WM_DRAWITEM()
    ON_WM_HELPINFO()
    ON_COMMAND(ID_HELP, OnHelp)
    ON_BN_CLICKED(IDC_FONT, OnFont)
    ON_BN_CLICKED(IDC_RADIOFONT, OnRadiofont)
    ON_BN_CLICKED(IDC_RADIOFONT2, OnRadiofont2)
    ON_BN_CLICKED(IDC_COLOR, OnColor)
    ON_BN_CLICKED(IDC_APPLY, OnApply)
    ON_BN_CLICKED(IDC_RESET, OnReset)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CColumnProp dialog


CColumnProp::CColumnProp(CWnd* pParent /*=NULL*/)
    : CDialog(CColumnProp::IDD, pParent)
{
    //{{AFX_DATA_INIT(CColumnProp)
    m_iHorzAlign = -1;
    m_iVertAlign = -1;
    m_iFont = -1;
    m_csTxt = _T("");
    m_csMsg = _T("");
    //}}AFX_DATA_INIT
    memset(&m_lfDefault, 0, sizeof(LOGFONT));
    memset(&m_lfCustom, 0, sizeof(LOGFONT));
    m_bDisableTxt=false;
    m_applytoall = false;
}


void CColumnProp::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CColumnProp)
    DDX_CBIndex(pDX, IDC_ALIGNHORZ, m_iHorzAlign);
    DDX_CBIndex(pDX, IDC_ALIGNVERT, m_iVertAlign);
    DDX_Radio(pDX, IDC_RADIOFONT, m_iFont);
    DDX_Text(pDX, IDC_ROWCOL_EDIT, m_csTxt);
    DDV_MaxChars(pDX, m_csTxt, 255);
    DDX_Text(pDX, IDC_ROWCOL_TEXT, m_csMsg);
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////
// CColumnProp message handlers


BOOL CColumnProp::OnInitDialog()
{
    CDialog::OnInitDialog();

    CString cs;
    cs.Format(IDS_DEFAULTFONT, PortableFont(m_lfDefault).GetDescription().GetString());
    GetDlgItem(IDC_RADIOFONT)->SetWindowText(cs);

    GetDlgItem(IDC_FONT)->EnableWindow(m_iFont==1?true:false);
    GetDlgItem(IDC_ROWCOL_EDIT)->EnableWindow(!m_bDisableTxt);
    GetDlgItem(IDC_ROWCOL_TEXT)->EnableWindow(!m_bDisableTxt);
    SetWindowText(m_csDlgTitle);

    return TRUE;
}


void CColumnProp::OnFont()
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
    dlg.m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
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
        m_color = dlg.GetColor();
        LOGFONT lfSelected = *dlg.m_cf.lpLogFont;
        if(memcmp((void*)&m_lfCustom,(void*)&lfSelected,sizeof(LOGFONT)) !=0 ) {
            m_lfCustom = *dlg.m_cf.lpLogFont;
        }
    }
    GetDlgItem(IDOK)->SetFocus();
}


void CColumnProp::OnRadiofont()
{
    GetDlgItem(IDC_FONT)->EnableWindow(FALSE);
}

void CColumnProp::OnRadiofont2()
{
    GetDlgItem(IDC_FONT)->EnableWindow();
}





void CColumnProp::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if  (nIDCtl == IDC_COLOR)
    {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);
        dc.FillSolidRect( &lpDrawItemStruct->rcItem,m_color);//COLORREF
            dc.DrawEdge(&lpDrawItemStruct->rcItem, BDR_RAISEDOUTER ,BF_RECT);
//      else
//          dc.DrawEdge(&lpDrawItemStruct->rcItem, BDR_SUNKENOUTER ,BF_RECT);
    }
    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CColumnProp::OnColor()
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

void CColumnProp::OnApply()
{
    if (AfxMessageBox(_T("The color will be applied to all text on all forms.\nDo you want to continue?"),MB_YESNO|MB_ICONEXCLAMATION, 0) == IDYES)
    {
        m_applytoall = true;
        OnOK();
    }

}

void CColumnProp::OnReset()
{
    m_color = RGB(0,0,0);
    UpdateData(TRUE);
    GetDlgItem(IDC_COLOR)->Invalidate();

}

void CColumnProp::OnHelp()
{
    if (m_csMsg[0] == 'C') {
        AfxGetApp()->HtmlHelp (HID_BASE_RESOURCE + IDD_COLUMNPROP);
    }
    else {
        AfxGetApp()->HtmlHelp (HID_BASE_RESOURCE + IDD_ROWPROP);
    }
}

BOOL CColumnProp::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    if (m_csMsg[0] == 'C') {
        AfxGetApp()->HtmlHelp (HID_BASE_RESOURCE + IDD_COLUMNPROP);
    }
    else {
        AfxGetApp()->HtmlHelp (HID_BASE_RESOURCE + IDD_ROWPROP);
    }
    return TRUE;
}
