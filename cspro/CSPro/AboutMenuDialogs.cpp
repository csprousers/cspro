#include "StdAfx.h"
#include "AboutMenuDialogs.h"


// --------------------------------------------------------------------------
// CTroubleshootingDialog
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CTroubleshootingDialog, CDialog)
    ON_WM_HELPINFO()
    ON_WM_PAINT()
END_MESSAGE_MAP()


BOOL CTroubleshootingDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_staticWWW.SubclassDlgItem(IDC_TROUBLESHOOTING_WEBSITE,this);
    m_staticWWW.SetText(_T("https://www.census.gov/data/software/cspro.html"));
    m_staticWWW.SetShellAction(_T("https://www.census.gov/data/software/cspro.html"));

    m_staticEmail.SubclassDlgItem(IDC_TROUBLESHOOTING_EMAIL,this);
    m_staticEmail.SetText(_T("cspro@lists.census.gov"));
    m_staticEmail.SetShellAction(_T("mailto:cspro@lists.census.gov"));

    m_staticCSProUsers.SubclassDlgItem(IDC_TROUBLESHOOTING_CSPROUSERS,this);
    m_staticCSProUsers.SetText(Html::CSProUsersForumUrl);
    m_staticCSProUsers.SetShellAction(Html::CSProUsersForumUrl);

    m_staticPack.SubclassDlgItem(IDC_TROUBLESHOOTING_PACK,this);
    m_staticPack.SetText(_T("Pack Application"));
    m_staticPack.SetMessageAction(ID_TOOLS_PACK);

    return TRUE;
}


BOOL CTroubleshootingDialog::OnHelpInfo(HELPINFO*)
{
    return TRUE; // there's no help for the about dialog box
}



// --------------------------------------------------------------------------
// CTransparentStaticImage
// --------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CTransparentStaticImage, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()


void CTransparentStaticImage::OnPaint() // 20120523
{
    HBITMAP hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(m_resourceID),IMAGE_BITMAP,0,0,LR_DEFAULTSIZE);

    if( hBitmap )
    {
        BITMAP bm;
        ::GetObject(hBitmap,sizeof(bm),&bm);

        CPaintDC dc(this);

        CDC memDC;
        memDC.CreateCompatibleDC(&dc);

        HANDLE hOldObject = memDC.SelectObject(hBitmap);
        dc.TransparentBlt(0,0,bm.bmWidth,bm.bmHeight,&memDC,0,0,bm.bmWidth,bm.bmHeight,m_transparentBit);
        memDC.SelectObject(hOldObject);

        DeleteObject(hBitmap);
    }
}



// --------------------------------------------------------------------------
// CAboutDialog
// --------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CAboutDialog, CDialogEx)

BEGIN_MESSAGE_MAP(CAboutDialog, CDialogEx)
    ON_WM_CTLCOLOR()
    ON_WM_PAINT()
    ON_WM_HELPINFO()
END_MESSAGE_MAP()


CAboutDialog::CAboutDialog(CWnd* pParent /*=NULL*/)
    :   CDialogEx(CAboutDialog::IDD, pParent),
        m_pLargeFont(CreateCustomFont(32, true)),
        m_pMediumFont(CreateCustomFont(14, true)),
        m_pSmallFont(CreateCustomFont(12, false))
{    
}


std::unique_ptr<CFont> CAboutDialog::CreateCustomFont(int size, bool bold) // 20120523
{
    auto pFont = std::make_unique<CFont>();

    if( pFont->CreateFont(-1 * size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, _T("Segoe UI")) )
    {
        return pFont;
    }

    return nullptr;
}


BOOL CAboutDialog::OnInitDialog() // 20120523
{
    CDialogEx::OnInitDialog();

    m_whiteBackground.Create(_T(""),
                             WS_CHILD | WS_VISIBLE, CRect(0, 0, 840, 177), this);

    m_title.Create(_T("Census and Survey\n")
                   _T("Processing System"),
                   WS_CHILD | WS_VISIBLE, CRect(10, 0, 300, 150), this);

    if( m_pLargeFont != nullptr )
        m_title.SetFont(m_pLargeFont.get());

    m_version.Create(FormatText(_T("Version %s\n%s"), Versioning::GetVersionDetailedString().GetString(), Versioning::GetReleaseDateString().GetString()),
                     WS_CHILD | WS_VISIBLE, CRect(10, 100, 300, 150), this);

    if( m_pMediumFont != nullptr )
        m_version.SetFont(m_pMediumFont.get());

    m_developers.Create(_T("CSPro is developed by the U.S. Census Bureau, ICF, and Serpro S.A.,\n")
                        _T("with funding from the U.S. Agency for International Development."),
                        WS_CHILD | WS_VISIBLE, CRect(10, 180, 440, 215), this);

    if( m_pSmallFont != nullptr )
        m_developers.SetFont(m_pSmallFont.get());

    // show the licenses only if the file exists
    std::wstring licenses_filename = PortableFunctions::PathAppendToPath(CSProExecutables::GetApplicationDirectory(), _T("Licenses.html"));

    if( PortableFunctions::FileIsRegular(licenses_filename) )
    {
        m_licenses.Create(NULL, WS_CHILD | WS_VISIBLE | SS_NOTIFY, CRect(10, 218, 440, 233), this);
        m_licenses.SetText(_T("View licenses for CSPro and the open-source software that it uses."));

        if( m_pSmallFont != nullptr )
            m_licenses.SetFont(m_pSmallFont.get());

        m_licenses.SetAction(
            [licenses_filename]()
            {
                Viewer viewer;
                viewer.UseEmbeddedViewer()
                      .SetTitle(_T("CSPro Licenses"))
                      .ViewFile(licenses_filename);
            });
    }

    m_logo.SetBitmap(IDB_ABOUT_LOGO, RGB(255, 255, 255));
    m_logo.Create(_T(""), WS_CHILD | WS_VISIBLE, CRect(340, 5, 500, 300), this);

    return TRUE;
}


HBRUSH CAboutDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) // 20120523
{
    HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    if( pWnd == &m_whiteBackground )
    {
        return (HBRUSH)GetStockObject(WHITE_BRUSH);
    }

    else if( pWnd == &m_title || pWnd == &m_version )
    {
        pDC->SetBkMode(TRANSPARENT);
        return (HBRUSH)GetStockObject(NULL_BRUSH);
    }

    else if( pWnd == &m_logo )
    {
        pDC->SetBkMode(TRANSPARENT);
        return (HBRUSH)GetStockObject(BLACK_BRUSH);
    }

    return hbr;
}


BOOL CAboutDialog::OnHelpInfo(HELPINFO*)
{
    return TRUE; // there's no help for the about dialog box
}
