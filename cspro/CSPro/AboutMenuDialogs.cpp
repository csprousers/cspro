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
    m_staticWWW.SetText(L"https://www.census.gov/data/software/cspro.html");
    m_staticWWW.SetShellAction(L"https://www.census.gov/data/software/cspro.html");

    m_staticEmail.SubclassDlgItem(IDC_TROUBLESHOOTING_EMAIL,this);
    m_staticEmail.SetText(L"cspro@lists.census.gov");
    m_staticEmail.SetShellAction(L"mailto:cspro@lists.census.gov");

    m_staticCSProUsers.SubclassDlgItem(IDC_TROUBLESHOOTING_CSPROUSERS,this);
    m_staticCSProUsers.SetText(Html::CSProUsersForumUrl);
    m_staticCSProUsers.SetShellAction(Html::CSProUsersForumUrl);

    m_staticPack.SubclassDlgItem(IDC_TROUBLESHOOTING_PACK,this);
    m_staticPack.SetText(L"Pack Application");
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
    :   CDialogEx(IDD_ABOUT, pParent),
        m_pLargeFont(CreateCustomFont(32, true)),
        m_pMediumFont(CreateCustomFont(14, true)),
        m_pSmallFont(CreateCustomFont(12, false))
{
}


std::unique_ptr<CFont> CAboutDialog::CreateCustomFont(int size, bool bold) // 20120523
{
    auto pFont = std::make_unique<CFont>();

    if( pFont->CreateFont(-1 * size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI") )
    {
        return pFont;
    }

    return nullptr;
}


BOOL CAboutDialog::OnInitDialog() // 20120523
{
    __super::OnInitDialog();

    m_whiteBackground.Create(L"",
                             WS_CHILD | WS_VISIBLE, CRect(0, 0, 840, 185), this);

    m_title.Create(L"Census and Survey\n"
                   L"Processing System",
                   WS_CHILD | WS_VISIBLE, CRect(10, 0, 300, 150), this);

    if( m_pLargeFont != nullptr )
        m_title.SetFont(m_pLargeFont.get());

    const std::string version_text = FormatText("Version %s\n%s", Versioning::GetVersionDetailedString().c_str(),
                                                                  Versioning::GetReleaseDateString().c_str());
    m_version.Create(TC::ToWide(version_text).c_str(), WS_CHILD | WS_VISIBLE, CRect(10, 100, 300, 150), this);

    if( m_pMediumFont != nullptr )
        m_version.SetFont(m_pMediumFont.get());

    m_developers.Create(L"CSPro is developed by the U.S. Census Bureau.",
                        WS_CHILD | WS_VISIBLE, CRect(10, 190, 440, 210), this);

    if( m_pSmallFont != nullptr )
        m_developers.SetFont(m_pSmallFont.get());

    // show the licenses only if the file exists
    const std::string licenses_file_path = Path::Combine(CSProExecutables::GetApplicationDirectory(), "Licenses.html");

    if( PortableFunctions::FileIsRegular(licenses_file_path) )
    {
        m_licenses.Create(NULL, WS_CHILD | WS_VISIBLE | SS_NOTIFY, CRect(10, 215, 440, 230), this);
        m_licenses.SetText(L"View licenses for CSPro and the open-source software that it uses.");

        if( m_pSmallFont != nullptr )
            m_licenses.SetFont(m_pSmallFont.get());

        m_licenses.SetAction(
            [licenses_file_path]()
            {
                Viewer viewer;
                viewer.UseEmbeddedViewer()
                      .SetTitle("CSPro Licenses")
                      .ViewFile(licenses_file_path);
            });
    }

    m_logo.SetBitmap(IDB_ABOUT_LOGO, RGB(255, 255, 255));
    m_logo.Create(L"", WS_CHILD | WS_VISIBLE, CRect(340, 5, 500, 300), this);

    return TRUE;
}


HBRUSH CAboutDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) // 20120523
{
    HBRUSH hbr = __super::OnCtlColor(pDC, pWnd, nCtlColor);

    if( pWnd == &m_whiteBackground )
    {
        return static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    }

    else if( pWnd == &m_title || pWnd == &m_version )
    {
        pDC->SetBkMode(TRANSPARENT);
        return static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    }

    else if( pWnd == &m_logo )
    {
        pDC->SetBkMode(TRANSPARENT);
        return static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    }

    return hbr;
}


BOOL CAboutDialog::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
    return TRUE; // there's no help for the about dialog box
}
