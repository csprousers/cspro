// FontPrefDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "FontPrefDlg.h"


// CFontPrefDlg dialog

IMPLEMENT_DYNAMIC(CFontPrefDlg, CDialog)
CStringArray m_acsFontFace;   // used by callback function to enumerate fonts

int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX* lpelfe, NEWTEXTMETRICEX* lpntme, int FontType, LPARAM lParam)
{
    CString cs(lpelfe->elfLogFont.lfFaceName);
    bool bFound=false;

    // don't add to the list if a font face is already present
    for (int i=0 ; i<m_acsFontFace.GetSize() ; i++) {
        if (m_acsFontFace[i]==cs)  {
            bFound=true;
            break;
        }
    }
    if (!bFound) {
        m_acsFontFace.Add(cs);
    }
    return 1;
}

CFontPrefDlg::CFontPrefDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFontPrefDlg::IDD, pParent)
{

}

void CFontPrefDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FONT_CMB, m_fontCmb);
    DDX_Control(pDX, IDC_ZOOMCMB, m_zoomCmb);
}


BEGIN_MESSAGE_MAP(CFontPrefDlg, CDialog)
    ON_BN_CLICKED(IDOK, &CFontPrefDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_RESET_BTN, &CFontPrefDlg::OnBnClickedResetBtn)
END_MESSAGE_MAP()


// CFontPrefDlg message handlers


void CFontPrefDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    CDialog::OnOK();
    if(m_zoomCmb.GetCurSel() >= 0) {
        CIMSAString zoom;
        m_zoomCmb.GetWindowText(zoom);
        zoom.TrimRight(_T("%"));
        int zoomPercent = zoom.Val();
        SetDesignerFontZoomLevel(zoomPercent);
    }
    else {
        SetDesignerFontZoomLevel(100);
    }
    CString font;
    if(m_fontCmb.GetCurSel() >= 0) {
        m_fontCmb.GetWindowText(font);
    }
    SetDesignerFontName(font);
}


void CFontPrefDlg::OnBnClickedResetBtn()
{
    // TODO: Add your control notification handler code here
    //Reset the registry keys for fontface name as blank and zoom as 100%
    m_fontCmb.SetCurSel(-1);
    m_zoomCmb.SetCurSel(-1);

}


BOOL CFontPrefDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //Populate the fonts

    CClientDC dc(this);

    m_fontCmb.ResetContent();
    LOGFONT lf = { 0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, _T("") };
    EnumFontFamiliesEx(dc.m_hDC, &lf, (FONTENUMPROC)EnumFontFamExProc, 0, 0 );

    for (int i=0 ; i<m_acsFontFace.GetSize() ; i++) {
        m_fontCmb.AddString(m_acsFontFace[i]);
    }
    m_acsFontFace.RemoveAll();

    //Poopulate the zoom level

    m_zoomCmb.ResetContent();
    m_zoomCmb.AddString(_T("100%"));
    m_zoomCmb.AddString(_T("125%"));
    m_zoomCmb.AddString(_T("150%"));
    m_zoomCmb.AddString(_T("175%"));
    m_zoomCmb.AddString(_T("200%"));

    CIMSAString zoomStr;
    zoomStr.Str(GetDesignerFontZoomLevel());
    m_zoomCmb.SelectString(-1,zoomStr + _T("%"));

    CString designerFontName = GetDesignerFontName();
    m_fontCmb.SelectString(-1,designerFontName);
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
