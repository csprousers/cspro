#include "StdAfx.h"
#include "OccDlg.h"


/////////////////////////////////////////////////////////////////////////////
// COccDlg dialog

COccDlg::COccDlg(CWnd* pParent /*=NULL*/)
    : CDialog(COccDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(COccDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_pDoc = NULL;
}


void COccDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COccDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COccDlg, CDialog)
    //{{AFX_MSG_MAP(COccDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COccDlg message handlers

BOOL COccDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(_T("Occurrence Labels (") + m_sLabel + _T(")"));

    CWnd* pWnd = GetDlgItem(IDC_OCCGRID);
    pWnd->GetClientRect(&m_rect);
    m_OccGrid.m_iCols = 1 ;
    m_OccGrid.m_iRows = m_OccGrid.m_Labels.GetSize();
    m_OccGrid.m_pDoc = m_pDoc;

    //Attach the grid to the control
    pWnd->SetFocus();
    m_OccGrid.AttachGrid(this,IDC_OCCGRID);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
