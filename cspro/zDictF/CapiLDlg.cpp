// CapiLDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "CapiLDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCapilangDlg dialog


CCapilangDlg::CCapilangDlg(CWnd* pParent /*=NULL*/)
    : CDialog(IDD_CAPI_LANGDLG, pParent)
{
    //{{AFX_DATA_INIT(CCapilangDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CCapilangDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCapilangDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCapilangDlg, CDialog)
    //{{AFX_MSG_MAP(CCapilangDlg)
    ON_WM_CLOSE()
    ON_WM_KEYDOWN()
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_MODIFY, OnModify)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCapilangDlg message handlers


BOOL CCapilangDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_lf.lfHeight           = 16;
    m_lf.lfWidth            = 0;
    m_lf.lfEscapement       = 0;
    m_lf.lfOrientation      = 0;
    m_lf.lfWeight           = FW_NORMAL;
    m_lf.lfItalic           = FALSE;
    m_lf.lfUnderline        = FALSE;
    m_lf.lfStrikeOut        = FALSE;
    m_lf.lfCharSet          = ANSI_CHARSET;
    m_lf.lfOutPrecision     = OUT_DEFAULT_PRECIS;
    m_lf.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
    m_lf.lfQuality          = DEFAULT_QUALITY;
    m_lf.lfPitchAndFamily   = FF_SWISS;

    lstrcpy(m_lf.lfFaceName,_T("Arial"));

    CRect rect;
    CWnd* pWnd = GetDlgItem(IDC_LANGGRID);
    pWnd->GetClientRect(&rect);
    m_Langgrid.SetGridFont(&m_lf);
    m_Langgrid.SetDefFont(m_Langgrid.AddFont(m_lf.lfFaceName, m_lf.lfHeight, m_lf.lfWeight));



    pWnd->SetFocus();
    m_Langgrid.AttachGrid(this,IDC_LANGGRID);

    //Show the grid
    m_Langgrid.SetFocus();
    m_Langgrid.ShowWindow(SW_SHOW);
    m_Langgrid.GotoCell(0,0);
    m_Langgrid.StartEdit();
    m_Langgrid.Update();

    UpdateData(TRUE);
    this->SetFocus();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CCapilangDlg::OnClose()
{
//  delete m_pRelation;
    CDialog::OnClose();
}

BOOL CCapilangDlg::PreTranslateMessage(MSG* pMsg)
{
    CWnd* pWnd = GetDlgItem(IDC_LANGGRID);
    CWnd* pWnd1 = GetFocus();
    if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE){
        if(this->m_Langgrid.GetSafeHwnd() && !this->m_Langgrid.IsEditing()) {
            OnCancel();
            return TRUE;
        }
    }
    if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB){
        if(this->m_Langgrid.GetSafeHwnd() && !this->m_Langgrid.IsEditing()) {
            return CDialog::PreTranslateMessage(pMsg);
        }
    }
    if ( (pWnd1 != NULL) && (pWnd->IsChild(pWnd1)))
    {
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
        {
            pWnd1->SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
            return TRUE;
        }
        else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE)
        {
            pWnd1->SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
            return TRUE;
        }
        else
            return pWnd->SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
    }
    else
    {
        if (pMsg->message == WM_KEYDOWN)
        {
            pWnd->SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
            return TRUE;
        }

        return CDialog::PreTranslateMessage(pMsg);
    }
}

void CCapilangDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CWnd* pWnd = GetDlgItem(IDC_LANGGRID);
    if (GetFocus() == pWnd)
        pWnd->SendMessage(WM_KEYDOWN,nChar,nRepCnt);
    else
        CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CCapilangDlg::OnOK()
{
    // csc 3/15/2004 ... end editing if a row was being worked on
    if (m_Langgrid.IsEditing()) {
        if (!m_Langgrid.EditEnd()) {
            // problem with the row being edited ... let the user fix it
            return;
        }
    }
    CDialog::OnOK();
}

void CCapilangDlg::OnEditAdd()
{
     if (m_Langgrid.IsEditing()) {
        if (!m_Langgrid.EditEnd()) {
            // problem with the row being edited ... let the user fix it
            return;
        }
    }
    m_Langgrid.OnEditAdd();
}

void CCapilangDlg::OnAdd()
{
     if (m_Langgrid.IsEditing()) {
        if (!m_Langgrid.EditEnd()) {
            // problem with the row being edited ... let the user fix it
            return;
        }
    }
    m_Langgrid.OnEditAdd();
}

void CCapilangDlg::OnDelete()
{
    m_Langgrid.OnEditDelete();
}

void CCapilangDlg::OnModify()
{
     if (m_Langgrid.IsEditing()) {
        if (!m_Langgrid.EditEnd()) {
            // problem with the row being edited ... let the user fix it
            return;
        }
    }
    m_Langgrid.OnEditModify();
}
