#include "StdAfx.h"
#include "TextFontDlg.h"


INT_PTR CTextFontDialog::DoModal()
{
    m_cf.Flags |= CF_ENABLETEMPLATE;
    m_cf.hInstance = ::GetModuleHandle(L"zFormF.dll");
    m_cf.lpTemplateName = MAKEINTRESOURCE(IDD_TEXTFONTDLG);

    return CFontDialog::DoModal();
}
