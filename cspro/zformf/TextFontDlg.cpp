#include "StdAfx.h"
#include "TextFontDlg.h"


int CTextFontDialog::DoModal()
{
    m_cf.Flags |= CF_ENABLETEMPLATE;
    m_cf.hInstance = ::GetModuleHandle(_T("zFormF.dll"));
    m_cf.lpTemplateName = MAKEINTRESOURCE(IDD_TEXTFONTDLG);

    return CFontDialog::DoModal();
}
