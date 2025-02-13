// facing.cpp : implementation file
//

#include "StdAfx.h"
#include "facing.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFacingPagesDlg dialog


CFacingPagesDlg::CFacingPagesDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CFacingPagesDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CFacingPagesDlg)
    m_sHorz = _T("");
    m_sVert = _T("");
    //}}AFX_DATA_INIT
}


void CFacingPagesDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFacingPagesDlg)
    DDX_CBString(pDX, IDC_HORZ, m_sHorz);
    DDX_CBString(pDX, IDC_VERT, m_sVert);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFacingPagesDlg, CDialog)
    //{{AFX_MSG_MAP(CFacingPagesDlg)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFacingPagesDlg message handlers
