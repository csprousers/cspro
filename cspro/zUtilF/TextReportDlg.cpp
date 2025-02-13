#include "StdAfx.h"
#include "TextReportDlg.h"


IMPLEMENT_DYNAMIC(TextReportDlg, CDialog)

BEGIN_MESSAGE_MAP(TextReportDlg, CDialog)
    ON_BN_CLICKED(IDC_COPY_TEXT_TO_CLIPBOARD, &OnBnClickedCopyToClipboard)
END_MESSAGE_MAP()


TextReportDlg::TextReportDlg(const CString& heading, const CString& content, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_TEXT_REPORT, pParent),
        m_heading(heading),
        m_content(content)
{
}


void TextReportDlg::UseFixedWidthFont()
{
    // the font will be fully created in OnInitDialog
    m_fixedWidthFont = std::make_unique<CFont>();
}


void TextReportDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_TEXT_REPORT_HEADING, m_heading);
    DDX_Text(pDX, IDC_TEXT_REPORT_CONTENT, m_content);
}


BOOL TextReportDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // use a fixed width font if necessary
    if( m_fixedWidthFont != nullptr )
    {
        CWnd* content_window = GetDlgItem(IDC_TEXT_REPORT_CONTENT);

        LOGFONT lf;
        content_window->GetFont()->GetLogFont(&lf);

        lstrcpy(lf.lfFaceName, _T("Courier New"));
        m_fixedWidthFont->CreateFontIndirect(&lf);

        content_window->SetFont(m_fixedWidthFont.get());
    }

    return TRUE;
}


void TextReportDlg::OnBnClickedCopyToClipboard()
{
    WinClipboard::PutText(this, m_content);
}
