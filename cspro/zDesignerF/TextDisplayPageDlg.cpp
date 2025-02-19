#include "StdAfx.h"
#include "TextDisplayPageDlg.h"


unsigned int TextDisplayPageDlg::GetDialogTemplateId()
{
    return IDD_PAGE_DISPLAY_TEXT;
}


TextDisplayPageDlg::TextDisplayPageDlg(std::wstring text, CWnd* const pParent/* = nullptr*/)
    :   CDialog(GetDialogTemplateId(), pParent),
        m_text(std::move(text))
{
}


TextDisplayPageDlg::TextDisplayPageDlg(const std::string_view text_sv, CWnd* const pParent/* = nullptr*/)
    :   TextDisplayPageDlg(TC::ToWide(text_sv), pParent)
{
}


void TextDisplayPageDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_TEXT, m_text);
}
