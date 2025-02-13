//***************************************************************************
//  File name: IMSADLG.CPP
//
//  Description:
//       Implementation of special dialog box classes
//
//  NOTE: Place change history in the *.h file.
//
//***************************************************************************

#include "StdAfx.h"
#include "imsaDlg.H"
#include "CustomFont.h"
#include <afxpriv.h>


/////////////////////////////////////////////////////////////////////////////
// CNoteDlg dialog

/*--- static member ---*/
static CRect    rcNoteDlg(0,0,0,0);     // where the last note dialog (CNoteDlg) was placed, static so that it applies accross all views


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg::CNoteDlg
//
/////////////////////////////////////////////////////////////////////////////

CNoteDlg::CNoteDlg(bool treat_help_button_as_clear, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_NOTEDLG, pParent),
        m_treatHelpButtonAsClear(treat_help_button_as_clear)
{
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg::DoDataExchange
//
/////////////////////////////////////////////////////////////////////////////

void CNoteDlg::DoDataExchange(CDataExchange* pDX) {

    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_NOTEEDIT, m_csNoteEdit);
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg::OnInitDialog
//
/////////////////////////////////////////////////////////////////////////////

BOOL CNoteDlg::OnInitDialog()
{
    BOOL bRetVal;
    bRetVal = CDialog::OnInitDialog();

    CEdit* pEdit = (CEdit*) GetDlgItem(IDC_NOTEEDIT);
    ASSERT_VALID(pEdit);

    // 20100621 to allow for the dynamic setting of fonts for the notes dialog

    UserDefinedFonts* pUserFonts = nullptr;
    AfxGetMainWnd()->SendMessage(WM_IMSA_GET_USER_FONTS, (WPARAM)&pUserFonts);

    if( pUserFonts != nullptr && pUserFonts->IsFontDefined(UserDefinedFonts::FontType::Notes) ) // user has defined a particular font
    {
        GetDlgItem(IDC_NOTEEDIT)->SetFont(pUserFonts->GetFont(UserDefinedFonts::FontType::Notes));
        SetFont(pUserFonts->GetFont(UserDefinedFonts::FontType::Notes));
    }

    SetWindowText(m_csTitle);
    if (rcNoteDlg.IsRectEmpty())  {
        CenterWindow();
    }
    else  {
        MoveWindow(rcNoteDlg);
    }

    if( m_treatHelpButtonAsClear )
        GetDlgItem(IDC_HELPBUTTON)->SetWindowText(_T("&Clear"));

    SetCapture();
    ReleaseCapture();

    return bRetVal;
}


BEGIN_MESSAGE_MAP(CNoteDlg, CDialog)
    //{{AFX_MSG_MAP(CNoteDlg)
    ON_BN_CLICKED(IDC_HELPBUTTON, OnHelpbutton)
    ON_WM_SHOWWINDOW()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg::OnHelpbutton
//
/////////////////////////////////////////////////////////////////////////////

void CNoteDlg::OnHelpbutton()
{
    if( m_treatHelpButtonAsClear ) {
        m_csNoteEdit.Empty();
        UpdateData(FALSE);
    }

    else {
        AfxGetApp()->HtmlHelp(HID_BASE_RESOURCE + IDD_NOTEDLG);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg::OnOK
//
/////////////////////////////////////////////////////////////////////////////

void CNoteDlg::OnOK() {

    CDialog::OnOK();
    GetWindowRect(&rcNoteDlg);      // Save the window position

    if (SO::IsWhitespace(m_csNoteEdit)) {
        m_csNoteEdit.Empty();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg::OnCancel
//
/////////////////////////////////////////////////////////////////////////////

void CNoteDlg::OnCancel() {

    CDialog::OnCancel();
    GetWindowRect(&rcNoteDlg);      // Save the window position
}


/////////////////////////////////////////////////////////////////////////////
//
//                               CNoteDlg::OnShowWindow
//
/////////////////////////////////////////////////////////////////////////////

void CNoteDlg::OnShowWindow(BOOL bShow, UINT nStatus) {

    ((CEdit*) GetDlgItem(IDC_NOTEEDIT))->SetSel(-1,0);
    CDialog::OnShowWindow(bShow, nStatus);
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CHtmlStatic
//
//  Customized CStatic that acts like a hyperlink.
//  csc Jan 2005
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CHtmlStatic, CStatic)
    ON_WM_PAINT()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


CHtmlStatic::CHtmlStatic()
    :   m_action(CString())
{
}


void CHtmlStatic::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;

    GetClientRect(rcClient);
    CFont font;  // mimics default CStatic text
    font.CreateFont (-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, FF_DONTCARE,
                   _T("MS Shell Dlg"));
    CFont* pOldFont=dc.SelectObject(&font);
    dc.SetTextColor(rgbBlue);
    dc.SetBkMode(TRANSPARENT);

    dc.TextOut(0, 0, m_text);

    dc.SelectObject(pOldFont);
    font.DeleteObject();
}


BOOL CHtmlStatic::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest==HTCLIENT && pWnd==this)  {
        HCURSOR hCursor=::LoadCursor(NULL, IDC_HAND);
        if (hCursor==NULL) {
            hCursor=::LoadCursor(NULL, IDC_ARROW);
        }
        ::SetCursor(hCursor);
        return TRUE;
    }
    return CStatic::OnSetCursor(pWnd, nHitTest, message);
}

/////////////////////////////////////////////////////////////////////////////
//
//                             OnLButtonUp
// Acts like a click, and performs the HTML action,
//
/////////////////////////////////////////////////////////////////////////////
void CHtmlStatic::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
    if( std::holds_alternative<CString>(m_action) )
    {
        if( !std::get<CString>(m_action).IsEmpty() )
            ShellExecute(nullptr, nullptr, std::get<CString>(m_action), nullptr, nullptr, SW_SHOW);
    }

    else if( std::holds_alternative<unsigned>(m_action) )
    {
        AfxGetMainWnd()->SendMessage(WM_COMMAND, std::get<unsigned>(m_action));
    }

    else
    {
        std::get<std::function<void()>>(m_action)();
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAAboutDlg (App About)
//
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CIMSAAboutDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_HELPINFO()
END_MESSAGE_MAP()


CIMSAAboutDlg::CIMSAAboutDlg(const wstring_view module_name_sv/* = wstring_view()*/, HICON hIcon/* = nullptr*/)
    :   CDialog(IDD_ABOUTBOX),
        m_csModuleName(module_name_sv),
        m_hIcon(hIcon)
{
}


BOOL CIMSAAboutDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Prep hyperlinks
    m_staticWWW.SubclassDlgItem(IDC_STATIC_WWW, this);
    m_staticWWW.SetText(_T("www.census.gov/data/software/cspro.html"));
    m_staticWWW.SetShellAction(_T("https://www.census.gov/data/software/cspro.html"));
    m_staticEmail.SubclassDlgItem(IDC_STATIC_EMAIL, this);
    m_staticEmail.SetText(_T("CSPro@lists.census.gov"));
    m_staticEmail.SetShellAction(_T("mailto:cspro@lists.census.gov"));

    // Show caption
    SetWindowText(_T("About ") + m_csModuleName);

    // Show module name, version, and release date in about box
    CString csModule = m_csModuleName;

    if( csModule.Find(_T("CSPro")) != 0 )
        csModule.Insert(0, _T("CSPro "));

    GetDlgItem(IDC_MODULE)->SetWindowText(csModule);

    CString csVersion;
    GetDlgItem(IDC_VERSION)->GetWindowText(csVersion);
    csVersion.Append(Versioning::GetVersionDetailedString());
    GetDlgItem(IDC_VERSION)->SetWindowText(csVersion);

    GetDlgItem(IDC_VERSION_DATE)->SetWindowText(Versioning::GetReleaseDateString());

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CIMSAAboutDlg::OnPaint()
{
    // put application's icon into the dialog box

    CPaintDC dc(this); // device context for painting

    if (!IsIconic()) {
        CWnd* pIcon = GetDlgItem(IDI_APPICON);
        ASSERT_VALID(pIcon);
        CRect rcIcon;
        pIcon->GetWindowRect(&rcIcon);
        ScreenToClient(&rcIcon);
        dc.DrawIcon(rcIcon.left, rcIcon.top, m_hIcon);
    }

    // Do not call CDialog::OnPaint() for painting messages
}


BOOL CIMSAAboutDlg::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    return true; // 20090924 there's no help for the about dialog box
}
