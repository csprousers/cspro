// FmtFontD.cpp : implementation file
//

#include "StdAfx.h"
#include "FmtFontD.h"

namespace {

// FontDlgControlVals
// This helper class allows you set and get the values of the font dialogs UI controls.
// It is part of the big work around for the fact that WM_CHOOSEFONT_SETLOGFONT does
// do as advertised.  In order to reset the font dialog to the default font when the
// user clicks on the "use default" checkbox, we use this class.
class FontDlgControlVals {
public:

    void Get(HWND hWndDlg)
    {
        m_iFontName = ::SendMessage(GetDlgItem(hWndDlg,1136), CB_GETCURSEL, 0, 0);
        m_iFontStyle = ::SendMessage(GetDlgItem(hWndDlg,1137), CB_GETCURSEL, 0, 0);
        m_iFontSize = ::SendMessage(GetDlgItem(hWndDlg,1138), CB_GETCURSEL, 0, 0);
        m_iScript = ::SendMessage(GetDlgItem(hWndDlg,1140), CB_GETCURSEL, 0, 0);
        m_iUnderline = ::SendMessage(GetDlgItem(hWndDlg,1041), BM_GETCHECK, 0, 0);
    }

    void Set(HWND hWndDlg)
    {
        // not good enough to just change selections here, need to also send notify messages
        // otherwise font dialog doesn't get told of the changes
       ::SendMessage(GetDlgItem(hWndDlg,1136), CB_SETCURSEL, m_iFontName, 0);
       ::SendMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(1136, CBN_SELCHANGE), (LPARAM) GetDlgItem(hWndDlg,1136));
       ::SendMessage(GetDlgItem(hWndDlg,1137), CB_SETCURSEL, m_iFontStyle, 0);
       ::SendMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(1137, CBN_SELCHANGE), (LPARAM) GetDlgItem(hWndDlg,1137));
       ::SendMessage(GetDlgItem(hWndDlg,1138), CB_SETCURSEL, m_iFontSize, 0);
       ::SendMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(1138, CBN_SELCHANGE), (LPARAM) GetDlgItem(hWndDlg,1138));
       ::SendMessage(GetDlgItem(hWndDlg,1140), CB_SETCURSEL, m_iScript, 0);
       ::SendMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(1140, CBN_SELCHANGE), (LPARAM) GetDlgItem(hWndDlg,1140));
       ::SendMessage(GetDlgItem(hWndDlg,1041), BM_SETCHECK, m_iUnderline, 0);
       ::SendMessage(hWndDlg, WM_COMMAND, MAKEWPARAM(1041, BN_CLICKED), (LPARAM) GetDlgItem(hWndDlg,1041));
    }

    bool operator==(const FontDlgControlVals& rhs) const
    {
        return m_iFontName == rhs.m_iFontName &&
               m_iFontStyle == rhs.m_iFontStyle &&
               m_iFontSize == rhs.m_iFontSize &&
               m_iUnderline == rhs.m_iUnderline &&
               m_iScript == rhs.m_iScript;
    }

    bool operator!=(const FontDlgControlVals& rhs) const
    {
        return !(operator==(rhs));
    }

private:

    int m_iFontName;
    int m_iFontStyle;
    int m_iFontSize;
    int m_iUnderline;
    int m_iScript;
};

}

//////////////
//
// Globals - only way to get values into/outof hook proc
//
/////////////

FontDlgControlVals g_defDlgCtrls;
bool bUseDefault;
bool bShowUseDefault;

// CFHookProc
// Allows us to intercept messages for custom font dialog.
UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg,WPARAM wParam,LPARAM lParam)
{
    if(uiMsg == WM_INITDIALOG){

        if (!bShowUseDefault) {
            HWND hWnd = GetDlgItem(hdlg,IDC_USE_DEF_FONT);
            if(IsWindow(hWnd)){
                ShowWindow(hWnd, SW_HIDE);
            }
            hWnd = GetDlgItem(hdlg,IDC_USE_DEFAULT_GROUPBOX);
            if(IsWindow(hWnd)){
                ShowWindow(hWnd, SW_HIDE);
            }
        } else {

            // initial value of use default checkbox
            if (bUseDefault) {
                ::SendMessage(GetDlgItem(hdlg,IDC_USE_DEF_FONT), BM_SETCHECK, BST_CHECKED, 0);
            }
            else {
                ::SendMessage(GetDlgItem(hdlg,IDC_USE_DEF_FONT), BM_SETCHECK, BST_UNCHECKED, 0);
            }

#ifdef _DEBUG
            if (bUseDefault) {
                // make sure that if use default is set that initial values of UI are same
                // as those for default
                FontDlgControlVals dlgCtrls;
                dlgCtrls.Get(hdlg);
                ASSERT(dlgCtrls == g_defDlgCtrls);
            }
#endif

        }

        return 0;
    }
    else if ((uiMsg == WM_COMMAND && LOWORD(wParam) == 1136 && HIWORD(wParam) == CBN_SELCHANGE) ||
        (uiMsg == WM_COMMAND && LOWORD(wParam) == 1137 && HIWORD(wParam) == CBN_SELCHANGE) ||
        (uiMsg == WM_COMMAND && LOWORD(wParam) == 1138 && HIWORD(wParam) == CBN_SELCHANGE) ||
        (uiMsg == WM_COMMAND && LOWORD(wParam) == 1140 && HIWORD(wParam) == CBN_SELCHANGE) ||
        (uiMsg == WM_COMMAND && LOWORD(wParam) == 1041 && HIWORD(wParam) == BN_CLICKED)) {
        // one of the font controls changed - check to see if settings of the UI controls
        // are same as default, if not then uncheck the use default box
        FontDlgControlVals dlgCtrls;
        dlgCtrls.Get(hdlg);
        if (dlgCtrls != g_defDlgCtrls) {
           ::SendMessage(GetDlgItem(hdlg,IDC_USE_DEF_FONT), BM_SETCHECK, BST_UNCHECKED, 0);
           ::SendMessage(hdlg, WM_COMMAND, MAKEWPARAM(IDC_USE_DEF_FONT, BN_CLICKED), (LPARAM) GetDlgItem(hdlg,IDC_USE_DEF_FONT));
        }

        return 0;
    }

    return 0;
}

// CFHookProcGetDefaults
// Special hook proc used to grab default control settings for the default font.
UINT_PTR CALLBACK CFHookProcGetDefaults(HWND hdlg, UINT uiMsg,WPARAM wParam,LPARAM lParam)
{
    if(uiMsg == WM_INITDIALOG) {
        g_defDlgCtrls.Get(hdlg); // grab default settings
        ::PostMessage(hdlg, WM_COMMAND, IDABORT, 0); // force immediate dialog abort
        return 0;
    }

    return 0;
}

// CFmtFontDlg dialog

IMPLEMENT_DYNAMIC(CFmtFontDlg, CFontDialog)

CFmtFontDlg::CFmtFontDlg(LPLOGFONT lplfInitial, DWORD dwFlags, CDC* pdcPrinter, CWnd* pParentWnd) :
    CFontDialog(lplfInitial, dwFlags, pdcPrinter, pParentWnd)
{
     m_bUseDefault=FALSE;
     m_bShowUseDefault = TRUE;
}


BEGIN_MESSAGE_MAP(CFmtFontDlg, CFontDialog)
    //{{AFX_MSG_MAP(CFmtFontDlg)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
    ON_BN_CLICKED(IDC_USE_DEF_FONT, OnBnClickedUseDefFont)
END_MESSAGE_MAP()



CFmtFontDlg::~CFmtFontDlg()
{
}

void CFmtFontDlg::DoDataExchange(CDataExchange* pDX)
{
    CFontDialog::DoDataExchange(pDX);
}

INT_PTR CFmtFontDlg::DoModal()
{
    // set up up template and hook proc - std stuff for custom font dialog
    m_cf.Flags |= CF_ENABLETEMPLATE|CF_ENABLEHOOK|CF_INITTOLOGFONTSTRUCT;
    m_cf.hInstance = ::GetModuleHandle(_T("zTableF.dll"));
    m_cf.lpTemplateName = MAKEINTRESOURCE(IDD_FMT_FONTDLG);

    // set global data used by hook proc
    !m_bUseDefault ? bUseDefault = false : bUseDefault = true;
    !m_bShowUseDefault ? bShowUseDefault = false : bShowUseDefault = true;

    // create dialog once with a special hook proc in order to get the correct UI settings
    // for the default font.  This special hook proc gets the defaults and then immediately
    // aborts the dialog so user never sees the dialog.
    LOGFONT* lpLf = m_cf.lpLogFont;
    m_cf.lpLogFont = &m_lfDef; // use default font, not current font
    m_cf.lpfnHook = CFHookProcGetDefaults; // use special hook proc
    CFontDialog::DoModal();

    // now run the dialog for real with the regular hook proc
    m_cf.lpLogFont = lpLf;
    m_cf.lpfnHook = CFHookProc;

    return CFontDialog::DoModal();
}

void CFmtFontDlg::OnBnClickedUseDefFont()
{
    if(((CButton*)GetDlgItem(IDC_USE_DEF_FONT))->GetCheck()){
        m_bUseDefault=TRUE;
        g_defDlgCtrls.Set(GetSafeHwnd());
        ((CButton*)GetDlgItem(IDC_USE_DEF_FONT))->SetCheck(BST_CHECKED);
    }
    else {
        m_bUseDefault = FALSE;
    }
}
