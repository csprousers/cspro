// LeftProp.cpp : implementation file
//

#include "StdAfx.h"
#include "leftprop.h"
#include "MainFrm.h"
#include "RunView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftPropSheet

IMPLEMENT_DYNAMIC(CLeftPropSheet, CPropertySheet)

CLeftPropSheet::CLeftPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
        :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{

        AddPage( &m_page1 );

}

CLeftPropSheet::CLeftPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
        :CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
        AddPage( &m_page1 );

}

CLeftPropSheet::~CLeftPropSheet()
{
}


BEGIN_MESSAGE_MAP(CLeftPropSheet, CPropertySheet)
        //{{AFX_MSG_MAP(CLeftPropSheet)
        ON_WM_SIZE()
        ON_WM_LBUTTONDOWN()
        ON_WM_KILLFOCUS()
        ON_WM_SETFOCUS()
        ON_WM_CREATE()
        //}}AFX_MSG_MAP


END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftPropSheet message handlers

bool CLeftPropSheet::CreateCaseView(CCreateContext* pContext)
{
    return m_page1.CreateCaseView(pContext);
}

bool CLeftPropSheet::CreateCaseTree()
{
    CWnd* pMainWnd = AfxGetMainWnd();
    if(pMainWnd && IsWindow(pMainWnd->GetSafeHwnd())){
        CEntryrunDoc*   pDoc            = (CEntryrunDoc*) (((CMainFrame*)pMainWnd)->GetActiveDocument());
            CDEItemBase*    pCurItemBase        = pDoc->GetCurField();
        if(!pCurItemBase){
            return false;
        }
    }

        bool bActivate = ((CMainFrame*)pMainWnd)->m_bCaseTreeActiveOnStart;

    int iPageIdx = GetPageIndex(&m_page2);
    if(iPageIdx<0){
        AddPage(&m_page2);
        SetActivePage(&m_page2);
    }

    bool    bCreateOK = (m_page2.GetCaseTree()!=NULL);

    if(!bActivate) {
        GetTabControl()->SetCurSel(0);
        SetActivePage(&m_page1);
    }

        if(!bCreateOK){
                RemovePage(&m_page2);
                SetActivePage(&m_page1);
        }

    return bCreateOK;

}

void CLeftPropSheet::DeleteCaseTree()
{
    m_page2.DeleteCaseTree();

    int iPageIdx = GetPageIndex(&m_page2);
    if(iPageIdx>=0){
        RemovePage(iPageIdx);
    }

    CWnd* pMainWnd = AfxGetMainWnd();
    if(pMainWnd && IsWindow(pMainWnd->GetSafeHwnd())){
        pMainWnd->PostMessage(UWM::CSEntry::CaseTreeFocus);
    }
}

CCaseView*      CLeftPropSheet::GetCaseView()
{
        return m_page1.GetCaseView();
}

CCaseTree*      CLeftPropSheet::GetCaseTree()
{
        return m_page2.GetCaseTree();
}

void CLeftPropSheet::OnSize(UINT nType, int cx, int cy)
{
    CPropertySheet::OnSize(nType, cx, cy);
    m_icx = cx;
    m_icy = cy;

    // TODO: Add your message handler code here

    //sizing the tab control
    CTabCtrl*       pTabCtrl = GetTabControl();
    if(pTabCtrl && IsWindow(pTabCtrl->GetSafeHwnd())){
                pTabCtrl->MoveWindow(0,0,cx,cy);
    }

    //sizing the active page
    CPropertyPage* pPage = GetActivePage ();
    if(pPage && IsWindow(pPage->GetSafeHwnd())){
                pPage->MoveWindow(4,22,cx-4 -4,cy-22-4);
    }
}


void CLeftPropSheet::OnLButtonDown(UINT nFlags, CPoint point)
{
    CPropertySheet::OnLButtonDown(nFlags, point);
}

void CLeftPropSheet::SetMainFrame( CMainFrame* pMainFrame )
{
    m_page2.SetMainFrame( pMainFrame );
}

void CLeftPropSheet::OnKillFocus(CWnd* pNewWnd)
{
    CPropertySheet::OnKillFocus(pNewWnd);
}

void CLeftPropSheet::OnSetFocus(CWnd* pOldWnd)
{
    CPropertySheet::OnSetFocus(pOldWnd);
}

int CLeftPropSheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
            return -1;

    return 0;
}

void CLeftPropSheet::DummyMove()
{
    CCaseTree* pTree = GetCaseTree();
    if(pTree){
        m_page2.DummyMove();
    }
}
