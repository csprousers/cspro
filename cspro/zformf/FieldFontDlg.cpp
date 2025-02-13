#include "StdAfx.h"
#include "FieldFontDlg.h"


namespace
{
    constexpr const TCHAR* FieldFontPrefix  = _T("Current font:  ");
    constexpr const TCHAR* SystemFontPrefix = _T("System font:  ");
}


BEGIN_MESSAGE_MAP(FieldFontDlg, CDialog)
    ON_BN_CLICKED(IDC_FIELD_FONT, OnFieldFont)
    ON_BN_CLICKED(IDC_FIELD_RESET, OnFieldReset)
END_MESSAGE_MAP()


FieldFontDlg::FieldFontDlg(PortableFont field_font, CWnd* pParent /*=NULL*/)
    :   CDialog(FieldFontDlg::IDD, pParent),
        m_fieldFont(std::move(field_font)),
        m_systemFieldFont(PortableFont::FieldDefault),
        m_systemFieldFontDescription(SystemFontPrefix + m_systemFieldFont.GetDescription())
{
}


void FieldFontDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    if( !pDX->m_bSaveAndValidate )
        m_fieldFontDescription = FieldFontPrefix + m_fieldFont.GetDescription();

    DDX_Text(pDX, IDC_FIELD_FONT_DESC, m_fieldFontDescription);
    DDX_Text(pDX, IDC_SYSTEM_FONT_DESC, m_systemFieldFontDescription);
}


void FieldFontDlg::OnFieldFont()
{
    LOGFONT logfont = m_fieldFont;

    CFontDialog dlg(&logfont, CF_SCREENFONTS);

    if( dlg.DoModal() != IDOK )
        return;

    m_fieldFont = *dlg.m_cf.lpLogFont;

    UpdateData(FALSE);
    GotoDlgCtrl(GetDlgItem(IDOK));
}


void FieldFontDlg::OnFieldReset()
{
    m_fieldFont = m_systemFieldFont;

    UpdateData(FALSE);
    GotoDlgCtrl(GetDlgItem(IDOK));
}
