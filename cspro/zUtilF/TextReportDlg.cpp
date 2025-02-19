#include "StdAfx.h"
#include "TextReportDlg.h"


BEGIN_MESSAGE_MAP(TextReportDlg, CDialog)
    ON_BN_CLICKED(IDC_COPY_TEXT_TO_CLIPBOARD, OnBnClickedCopyToClipboard)
END_MESSAGE_MAP()


TextReportDlg::TextReportDlg(std::string heading, std::string content, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_TEXT_REPORT, pParent),
        m_heading(std::move(heading)),
        m_content(std::move(content))
{
}


void TextReportDlg::UseFixedWidthFont()
{
    // the font will be fully created in OnInitDialog
    m_fixedWidthFont = std::make_unique<CFont>();
}


void TextReportDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_TEXT_REPORT_HEADING, m_heading);
    DDX_Text(pDX, IDC_TEXT_REPORT_CONTENT, m_content);
}


BOOL TextReportDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // use a fixed width font if necessary
    if( m_fixedWidthFont != nullptr )
    {
        CWnd* const content_window = GetDlgItem(IDC_TEXT_REPORT_CONTENT);

        LOGFONT lf;
        content_window->GetFont()->GetLogFont(&lf);

        lstrcpy(lf.lfFaceName, L"Courier New");
        m_fixedWidthFont->CreateFontIndirect(&lf);

        content_window->SetFont(m_fixedWidthFont.get());
    }

    return TRUE;
}


void TextReportDlg::OnBnClickedCopyToClipboard()
{
    WinClipboard::PutText(this, m_content);
}
