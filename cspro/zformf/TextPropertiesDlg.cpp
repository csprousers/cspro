#include "StdAfx.h"
#include "TextPropertiesDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CTxtPropDlg dialog


CTxtPropDlg::CTxtPropDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(CTxtPropDlg::IDD, pParent)
{
    m_pView = (CFormScrollView*)pParent;
    //{{AFX_DATA_INIT(CTxtPropDlg)
    m_iFont = -1;
    m_sText = _T("");
    m_applytoall = false;
    //}}AFX_DATA_INIT
}


void CTxtPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTxtPropDlg)
    DDX_Radio(pDX, IDC_RADIOFONT, m_iFont);

    if( pDX->m_bSaveAndValidate ) // 20130506
    {
        DDX_Text(pDX, IDC_LABEL, m_sText);
        m_sText = DelimitCRLF(m_sText);
    }

    else
    {
        CString csUndelimitedText = UndelimitCRLF(m_sText);
        DDX_Text(pDX, IDC_LABEL, csUndelimitedText);
    }

    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTxtPropDlg, CDialog)
    //{{AFX_MSG_MAP(CTxtPropDlg)
    ON_BN_CLICKED(IDC_RADIOFONT, OnRadiofont)
    ON_BN_CLICKED(IDC_RADIOFONT2, OnRadiofont2)
    ON_BN_CLICKED(IDC_FONT, OnFont)
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_COLOR, OnColor)
    ON_BN_CLICKED(IDC_APPLY, OnApply)
    ON_BN_CLICKED(IDC_RESET, OnReset)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTxtPropDlg message handlers

BOOL CTxtPropDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    if(!m_bEnableText){
        GetDlgItem(IDC_LABEL)->EnableWindow(false);
    }

    CString cs;
    cs.Format(IDS_DEFAULTFONT, PortableFont(m_lfDefault).GetDescription().GetString());
    GetDlgItem(IDC_RADIOFONT)->SetWindowText(cs);

    GetDlgItem(IDC_FONT)->EnableWindow(m_iFont==0?false:true);

    if( m_iFont ) // 20120612 if they have a custom font selected, let's display the descriptive name of it
    {
        cs.Format(IDS_CUSTOMFONT, PortableFont(m_lfCustom).GetDescription().GetString());
        GetDlgItem(IDC_RADIOFONT2)->SetWindowText(cs);
    }

    CFormDoc* pDoc = m_pView->GetDocument();

    if(pDoc->GetFormFile().GetDefaultTextFont().IsArabic()) {
        GetDlgItem(IDC_LABEL)->SetFont(&pDoc->GetFormFile().GetDefaultTextFont().GetCFont());
    }

    return TRUE;
}

void CTxtPropDlg::OnRadiofont()
{
    GetDlgItem(IDC_FONT)->EnableWindow(FALSE);
}

void CTxtPropDlg::OnRadiofont2()
{
    GetDlgItem(IDC_FONT)->EnableWindow();
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CTxtPropDlg::OnFont()
//
/////////////////////////////////////////////////////////////////////////////////
void CTxtPropDlg::OnFont()
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
    CTextFontDialog dlg(&lfFont, CF_SCREENFONTS|CF_EFFECTS);//CF_SCREENFONTS
    dlg.m_cf.rgbColors = m_color;
    if (dlg.DoModal()==IDOK)  {
        // 20120612 i got rid of the following line because color isn't available in the font dialog box
        // m_color = dlg.GetColor();
        LOGFONT lfSelected = *dlg.m_cf.lpLogFont;
        if(memcmp((void*)&m_lfCustom,(void*)&lfSelected,sizeof(LOGFONT)) !=0 ) {
            m_lfCustom = *dlg.m_cf.lpLogFont;
        }

        CString cs; // 20120612
        cs.Format(IDS_CUSTOMFONT, PortableFont(m_lfCustom).GetDescription().GetString());
        GetDlgItem(IDC_RADIOFONT2)->SetWindowText(cs);
    }
}

void CTxtPropDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
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

void CTxtPropDlg::OnColor()
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

void CTxtPropDlg::OnApply()
{
    if (AfxMessageBox(_T("The color will be applied to all text on all forms.\nDo you want to continue?"),MB_YESNO|MB_ICONEXCLAMATION, 0) == IDYES)
    {
        m_applytoall = true;
        OnOK();
    }
}


void CTxtPropDlg::OnReset()
{
    m_color = RGB(0,0,0);
    UpdateData(TRUE);
    GetDlgItem(IDC_COLOR)->Invalidate();
}
