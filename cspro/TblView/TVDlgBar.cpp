// TDlgBar.cpp : implementation file
//

#include "StdAfx.h"
#include "TVDlgBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTVDlgBar dialog


CTVDlgBar::CTVDlgBar()
    : COXSizeDialogBar(SZBARF_STDMOUSECLICKS)
{
    //{{AFX_DATA_INIT(CTVDlgBar)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CTVDlgBar::DoDataExchange(CDataExchange* pDX)
{
    COXSizeDialogBar::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTVDlgBar)
    DDX_Control(pDX, IDC_TABLETREE, m_TblTree);
    //}}AFX_DATA_MAP
}


/////////////////////////////////////////////////////////////////////////////////
//
//  BOOL CTVDlgBar::OnInitDialog()
//
/////////////////////////////////////////////////////////////////////////////////
BOOL CTVDlgBar::OnInitDialog()
{
    UpdateData(FALSE);
    m_TblTree.InitImageList();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
