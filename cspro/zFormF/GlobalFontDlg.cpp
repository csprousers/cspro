#include "StdAfx.h"
#include "GlobalFontDlg.h"


BEGIN_MESSAGE_MAP(CGlobalFDlg, CDialog)
    ON_BN_CLICKED(IDC_FONTRADIO1, OnRadio1)
    ON_BN_CLICKED(IDC_FONTRADIO2, OnRadio2)
    ON_BN_CLICKED(IDC_FONT, OnFont)
    ON_BN_CLICKED(IDC_APPLY, OnApply)
END_MESSAGE_MAP()


CGlobalFDlg::CGlobalFDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_GLOBALFONTDLG, pParent),
        m_iFont(-1),
        m_pFormView(nullptr)
{
}


void CGlobalFDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Radio(pDX, IDC_FONTRADIO1, m_iFont);
    DDX_Text(pDX, IDC_FONTDESC, m_sCurFontDesc);
}


void CGlobalFDlg::OnRadio1()
{
    GetDlgItem(IDC_FONT)->EnableWindow(FALSE);
}


void CGlobalFDlg::OnRadio2()
{
    GetDlgItem(IDC_FONT)->EnableWindow();
}


void CGlobalFDlg::OnFont()
{
    LOGFONT lfFont;
    LOGFONT lfNull;

    UpdateData();
    ASSERT(m_iFont==1);
    ASSERT(m_lfDefault.lfHeight!=0);
    memset((void*)&lfNull, 0, sizeof(LOGFONT));
    if (memcmp((void*)&m_lfSelectedFont, (void*)&lfNull, sizeof(LOGFONT))==0)  {
        // no custom font info available, start with default
        lfFont = m_lfDefault;
    }
    else  {
        // use custom font info
        lfFont = m_lfSelectedFont;
    }

    CTextFontDialog dlg(&lfFont, CF_SCREENFONTS|CF_EFFECTS);   // csc 4/14/2004
    if (dlg.DoModal()==IDOK)  {
        LOGFONT lfSelected = *dlg.m_cf.lpLogFont;
        if(memcmp((void*)&m_lfSelectedFont,(void*)&lfSelected,sizeof(LOGFONT)) !=0 ) {
            m_lfSelectedFont = *dlg.m_cf.lpLogFont;
        }
    }
}


void CGlobalFDlg::OnApply()
{
    if( AfxMessageBox(L"Are you sure you want to apply the font to all items?",
                      MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 ) == IDNO )
    {
        return;
    }

    ASSERT_VALID (m_pFormView);
    UpdateData(TRUE);
    if(m_iFont ==0){
        m_pFormView->ChangeFont(m_lfDefault);
    }
    else {
        m_pFormView->ChangeFont(m_lfSelectedFont);
    }
}


BOOL CGlobalFDlg::OnInitDialog()
{
    __super::OnInitDialog();

    m_sCurFontDesc = PortableFont(m_lfCurrentFont).GetDescription();
    UpdateData(FALSE);

    WindowsUtf8::SetText(this, IDC_FONTRADIO1, FormatText("&Reset to system default font (%s)",
                                                          PortableFont(m_lfDefault).GetDescription().c_str()));

    GetDlgItem(IDC_FONT)->EnableWindow(( m_iFont != 0 ));

    return TRUE;
}
