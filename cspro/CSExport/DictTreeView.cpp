// DictTreeView.cpp : implementation file
//

#include "StdAfx.h"
#include "CSExport.h"
#include "DictTreeView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDictTreeView

IMPLEMENT_DYNCREATE(CDictTreeView, CView)

CDictTreeView::CDictTreeView()
{
    SetCtrl(NULL);
}

CDictTreeView::~CDictTreeView()
{
}


BEGIN_MESSAGE_MAP(CDictTreeView, CView)
    //{{AFX_MSG_MAP(CDictTreeView)
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDictTreeView drawing

void CDictTreeView::OnDraw(CDC* pDC)
{
    UNREFERENCED_PARAMETER(pDC);
    // TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CDictTreeView diagnostics

#ifdef _DEBUG
void CDictTreeView::AssertValid() const
{
    CView::AssertValid();
}

void CDictTreeView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDictTreeView message handlers


void CDictTreeView::SetCtrl(CTreeCtrl* pTreeCtrl){
    m_pTree = pTreeCtrl;
}

void CDictTreeView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);

    if(!m_pTree)
        return;

    if(!IsWindow(m_pTree->GetSafeHwnd()))
        return;

    m_pTree->MoveWindow(0,0,cx,cy);
}

void CDictTreeView::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    // GHM 20090901 this code removed because it was causing scrollbars to disappear in the
    // tree view upon loading a dictionary (if the dictionary items were too large for the
    // given window)
    /*if(!m_pTree)
        return;

    if(!IsWindow(m_pTree->GetSafeHwnd()))
        return;
    CRect rc;
    GetParentFrame()->GetWindowRect(rc);
    m_pTree->MoveWindow(0,0,rc.Width(),rc.Height());

    UpdateWindow();*/
}
