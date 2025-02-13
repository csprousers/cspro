#include "StdAfx.h"
#include "GlobalFontDlg.h"


BEGIN_MESSAGE_MAP(CGlobalFDlg, CDialog)
    ON_BN_CLICKED(IDC_FONTRADIO1, OnRadio1)
    ON_BN_CLICKED(IDC_FONTRADIO2, OnRadio2)
    ON_BN_CLICKED(IDC_FONT, OnFont)
    ON_BN_CLICKED(IDC_APPLY, OnApply)
END_MESSAGE_MAP()


CGlobalFDlg::CGlobalFDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(CGlobalFDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGlobalFDlg)
    m_iFont = -1;
    //}}AFX_DATA_INIT
    m_pFormView=NULL;
}


void CGlobalFDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGlobalFDlg)
    DDX_Radio(pDX, IDC_FONTRADIO1, m_iFont);
    DDX_Text(pDX, IDC_FONTDESC, m_sCurFontDesc);
    //}}AFX_DATA_MAP
}



/////////////////////////////////////////////////////////////////////////////
// CGlobalFDlg message handlers


/////////////////////////////////////////////////////////////////////////////////
//
//  void CGlobalFDlg::OnRadio1()
//
/////////////////////////////////////////////////////////////////////////////////
void CGlobalFDlg::OnRadio1()
{
    GetDlgItem(IDC_FONT)->EnableWindow(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CGlobalFDlg::OnRadio2()
//
/////////////////////////////////////////////////////////////////////////////////
void CGlobalFDlg::OnRadio2()
{
    GetDlgItem(IDC_FONT)->EnableWindow();
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CGlobalFDlg::OnFont()
//
/////////////////////////////////////////////////////////////////////////////////
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
    if(AfxMessageBox(_T("Are you sure you want to apply the font to all items?"),MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2) == IDNO){
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
    CDialog::OnInitDialog();

    m_sCurFontDesc = PortableFont(m_lfCurrentFont).GetDescription();
    UpdateData(FALSE);

    CString cs;
    cs.Format(IDS_SYSTEMFONT, PortableFont(m_lfDefault).GetDescription().GetString());
    GetDlgItem(IDC_FONTRADIO1)->SetWindowText(cs);

    GetDlgItem(IDC_FONT)->EnableWindow(m_iFont==0?false:true);

    return TRUE;
}
