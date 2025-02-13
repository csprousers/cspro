#include "StdAfx.h"
#include "RelDlg.h"


BEGIN_MESSAGE_MAP(CRelDlg, CDialog)
    ON_WM_CLOSE()
    ON_WM_KEYDOWN()
    ON_COMMAND(ID_EDIT_ADD, OnEditAdd)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_DELETE, OnDelete)
    ON_BN_CLICKED(IDC_INSERT, OnInsert)
END_MESSAGE_MAP()


CRelDlg::CRelDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(CRelDlg::IDD, pParent),
        m_pDoc(nullptr)
{
}


/////////////////////////////////////////////////////////////////////////////
// CRelDlg message handlers

BOOL CRelDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    //Attach the grid to the control
    m_lf.lfHeight           = (LONG)( 16 * ( GetDesignerFontZoomLevel() / 100.0 ) );
    m_lf.lfWidth            = 0;
    m_lf.lfEscapement       = 0;
    m_lf.lfOrientation      = 0;
    m_lf.lfWeight           = FW_NORMAL;
    m_lf.lfItalic           = FALSE;
    m_lf.lfUnderline        = FALSE;
    m_lf.lfStrikeOut        = FALSE;
    m_lf.lfCharSet          = DEFAULT_CHARSET/*ANSI_CHARSET*/;
    m_lf.lfOutPrecision     = OUT_DEFAULT_PRECIS;
    m_lf.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
    m_lf.lfQuality          = DEFAULT_QUALITY;
    m_lf.lfPitchAndFamily   = FF_SWISS;

    CString sFontName = GetDesignerFontName();
    sFontName.Trim();
    if(sFontName.IsEmpty()){
        lstrcpy(m_lf.lfFaceName,_T("Arial Unicode MS"));
    }
    else {
        lstrcpy(m_lf.lfFaceName,sFontName);
    }

    CRect rect;
    CWnd* pWnd = GetDlgItem(IDC_RELGRID);
    pWnd->GetClientRect(&rect);
    m_Relgrid.SetGridFont(&m_lf);
    m_Relgrid.SetDefRowHeight((int)( DEF_ROW_HEIGHT * (GetDesignerFontZoomLevel() / 100.0) ));

    m_Relgrid.SetDefFont(m_Relgrid.AddFont(m_lf.lfFaceName, m_lf.lfHeight, m_lf.lfWeight));
    m_Relgrid.SetDict(m_pDoc->GetDict());


    pWnd->SetFocus();
    m_Relgrid.AttachGrid(this,IDC_RELGRID);

    //Show the grid
    m_Relgrid.SendMessage(WM_SIZE);
    m_Relgrid.SetFocus();
    m_Relgrid.ShowWindow(SW_SHOW);
    m_Relgrid.GotoCell(0,0);
    m_Relgrid.StartEdit();
    m_Relgrid.Update();

    UpdateData(TRUE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CRelDlg::OnClose()
{
    if (m_Relgrid.IsEditing()) {
        m_Relgrid.EditQuit();
    }
    CDialog::OnClose();
}

BOOL CRelDlg::PreTranslateMessage(MSG* pMsg)
{
    CWnd* pWnd = GetDlgItem(IDC_RELGRID);
    CWnd* pWnd1 = GetFocus();

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
        {
            return pWnd->SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
        }
    }
    else
    {
        if (pMsg->message == WM_KEYDOWN )
        {
            pWnd->SendMessage(pMsg->message,pMsg->wParam,pMsg->lParam);
            return TRUE;
        }

        return CDialog::PreTranslateMessage(pMsg);
    }
}

void CRelDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CWnd* pWnd = GetDlgItem(IDC_RELGRID);
    if (GetFocus() == pWnd)
        pWnd->SendMessage(WM_KEYDOWN,nChar,nRepCnt);
    else
        CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CRelDlg::OnOK()
{
    if (m_Relgrid.IsEditing()) {
        if (!m_Relgrid.EditEnd()) {
            m_Relgrid.SetFocus();
            return;
        }
    }

    m_pDoc->GetDict()->SetRelations(std::move(m_Relgrid.m_dictRelations));
    m_pDoc->SetModified();

    CDialog::OnOK();
}

void CRelDlg::OnEditAdd()
{
    m_Relgrid.OnEditAdd();
}

void CRelDlg::OnAdd()
{
    m_Relgrid.OnEditAdd();
}

void CRelDlg::OnDelete()
{
    m_Relgrid.OnEditDelete();
}

void CRelDlg::OnInsert()
{
    m_Relgrid.OnEditInsert();
}

void CRelDlg::OnCancel()
{
    if (m_Relgrid.IsEditing()) {
        m_Relgrid.EditQuit();
    }
    CDialog::OnCancel();
}
