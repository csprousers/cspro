//***************************************************************************
//  File name: ImsaDlg.cpp
//
//  Description:
//       Implementation of special dialog box classes
//
//  NOTE: Place change history in the *.h file.
//
//***************************************************************************

#include "StdAfx.h"
#include "ImsaDlg.h"
#include "CustomFont.h"
#include "DataExchange.h"
#include "WindowsUtf8.h"
#include <afxpriv.h>


/////////////////////////////////////////////////////////////////////////////
// CNoteDlg dialog

/*--- static member ---*/
static CRect    rcNoteDlg(0,0,0,0);     // where the last note dialog (CNoteDlg) was placed, static so that it applies accross all views



// --------------------------------------------------------------------------
// CNoteDlg
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CNoteDlg, CDialog)
    ON_BN_CLICKED(IDC_HELPBUTTON, OnHelpButton)
    ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


CNoteDlg::CNoteDlg(const bool treat_help_button_as_clear, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_NOTEDLG, pParent),
        m_treatHelpButtonAsClear(treat_help_button_as_clear),
        m_useOnlyLF(true)
{
}


void CNoteDlg::SetNote(std::string note)
{
    ASSERT(m_useOnlyLF);

    m_note = std::move(note);

    const size_t first_newline_ch = m_note.find_first_of("\r\n");

    if( first_newline_ch != std::string::npos )
    {
        if( m_note.at(first_newline_ch) == '\r' || m_note.find('\r', first_newline_ch + 1) != std::string::npos )
        {
            m_useOnlyLF = false;
        }

        else
        {
            SO::MakeNewlineCRLF(m_note);
        }
    }
}


void CNoteDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_NOTEEDIT, m_note, true);
}


BOOL CNoteDlg::OnInitDialog()
{
    const BOOL result = __super::OnInitDialog();

    // 20100621 to allow for the dynamic setting of fonts for the notes dialog
    const UserDefinedFonts* const user_defined_fonts = UserDefinedFonts::GetUserDefinedFont(UserDefinedFonts::FontType::Notes);

    if( user_defined_fonts != nullptr )
    {
        GetDlgItem(IDC_NOTEEDIT)->SetFont(user_defined_fonts->GetFont(UserDefinedFonts::FontType::Notes));
        SetFont(user_defined_fonts->GetFont(UserDefinedFonts::FontType::Notes));
    }

    SetWindowText(m_title.c_str());

    if( rcNoteDlg.IsRectEmpty() )
    {
        CenterWindow();
    }

    else
    {
        MoveWindow(rcNoteDlg);
    }

    if( m_treatHelpButtonAsClear )
        GetDlgItem(IDC_HELPBUTTON)->SetWindowText(L"&Clear");

    SetCapture();
    ReleaseCapture();

    return result;
}


void CNoteDlg::OnOK()
{
    __super::OnOK();

    // Save the window position
    GetWindowRect(&rcNoteDlg);

    ASSERT(!SO::IsWhitespace(m_note) || m_note.empty());

    if( m_useOnlyLF )
        SO::MakeNewlineLF(m_note);
}


void CNoteDlg::OnCancel()
{
    __super::OnCancel();

    // Save the window position
    GetWindowRect(&rcNoteDlg);
}


void CNoteDlg::OnHelpButton()
{
    if( m_treatHelpButtonAsClear )
    {
        m_note.clear();
        UpdateData(FALSE);
    }

    else
    {
        AfxGetApp()->HtmlHelp(HID_BASE_RESOURCE + IDD_NOTEDLG);
    }
}



void CNoteDlg::OnShowWindow(const BOOL bShow, const UINT nStatus)
{
    static_cast<CEdit*>(GetDlgItem(IDC_NOTEEDIT))->SetSel(-1, 0);

    __super::OnShowWindow(bShow, nStatus);
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
                    L"MS Shell Dlg");
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
    m_staticWWW.SetText(L"www.census.gov/data/software/cspro.html");
    m_staticWWW.SetShellAction(L"https://www.census.gov/data/software/cspro.html");
    m_staticEmail.SubclassDlgItem(IDC_STATIC_EMAIL, this);
    m_staticEmail.SetText(L"CSPro@lists.census.gov");
    m_staticEmail.SetShellAction(L"mailto:cspro@lists.census.gov");

    // Show caption
    SetWindowText(L"About " + m_csModuleName);

    // Show module name, version, and release date in about box
    CString csModule = m_csModuleName;

    if( csModule.Find(L"CSPro") != 0 )
        csModule.Insert(0, L"CSPro ");

    GetDlgItem(IDC_MODULE)->SetWindowText(csModule);

    CString csVersion;
    GetDlgItem(IDC_VERSION)->GetWindowText(csVersion);

    const std::wstring detailed_version_text = TC::ToWide(Versioning::GetVersionDetailedString());
    csVersion.Append(detailed_version_text.c_str(), int32_cast(detailed_version_text.length()));

    GetDlgItem(IDC_VERSION)->SetWindowText(csVersion);

    WindowsUtf8::SetText(this, IDC_VERSION_DATE, Versioning::GetReleaseDateString());

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
