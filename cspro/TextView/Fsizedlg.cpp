// FSizeDlg.cpp : implementation file
//

#include "StdAfx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define HID_BASE_RESOURCE 0x00020000UL

/////////////////////////////////////////////////////////////////////////////
// CFontSizeDlg dialog


CFontSizeDlg::CFontSizeDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFontSizeDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CFontSizeDlg)
    m_csSize = _T("");
    //}}AFX_DATA_INIT
}


void CFontSizeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFontSizeDlg)
    DDX_CBString(pDX, IDC_FONT_SIZE, m_csSize);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFontSizeDlg, CDialog)
    //{{AFX_MSG_MAP(CFontSizeDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFontSizeDlg message handlers

BOOL CFontSizeDlg::OnInitDialog() {

    CDialog::OnInitDialog();
    m_csSize.Str(AfxGetApp()->GetProfileInt(_T("Options"), _T("FontSize"), 8));
    UpdateData(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CFontSizeDlg::OnOK() {

    UpdateData(TRUE);
    for (int i = 0 ; i < m_csSize.GetLength() ; i++) {
        if (!is_digit(m_csSize[i])) {
            MessageBox(_T("Size must be a number."), _T("Font Size"), MB_ICONINFORMATION | MB_OK);
            return;
        }
    }
    UINT uSize = (UINT) m_csSize.Val();
    if (uSize < 7) {
        uSize = 7;
    }
    if (uSize > 72) {
        uSize = 72;
    }
    m_csSize.Str(uSize);
    UpdateData(FALSE);
    AfxGetApp()->WriteProfileInt(_T("Options"), _T("FontSize"), uSize);
    CDialog::OnOK();
}
