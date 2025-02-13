// ExportSplitterFrame.cpp : implementation file
//

#include "StdAfx.h"
#include "ExportSplitterFrame.h"
#include "CSExport.h"
#include "DictTreeView.h"
#include "ExportOptionsView.h"
#include "ExptDoc.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExportSplitterFrame

IMPLEMENT_DYNCREATE(CExportSplitterFrame, CFrameWnd)

CExportSplitterFrame::CExportSplitterFrame()
{
    m_pLeftView  = NULL;
    m_pRightView = NULL;
    m_nIDHelp = IDR_MAINFRAME;
}

CExportSplitterFrame::~CExportSplitterFrame()
{
}


BEGIN_MESSAGE_MAP(CExportSplitterFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CExportSplitterFrame)
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportSplitterFrame message handlers


//called BEFORE OnCreateClient
void    CExportSplitterFrame::SetDoc(   CExportDoc* pDoc ){
    m_pDoc = pDoc;
}
CExportDoc* CExportSplitterFrame::GetDoc(){
    return m_pDoc;
}


BOOL CExportSplitterFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
    if (!m_cFlatSplitter.CreateStatic(this, 1, 2 ))
        return FALSE;

    if (!m_cFlatSplitter.CreateView(0,  0 , RUNTIME_CLASS(CDictTreeView), CSize(0, 0), pContext))
        return FALSE;

    if (!m_cFlatSplitter.CreateView(0,  1 , RUNTIME_CLASS(CExportOptionsView), CSize(0, 0), pContext))
        return FALSE;

    m_cFlatSplitter.SetColumnInfo(0, 300, 0);
    m_cFlatSplitter.SetColumnInfo(1, 405, 0);

    m_pLeftView     = (CDictTreeView*)      m_cFlatSplitter.GetPane(0,0);
    m_pRightView    = (CExportOptionsView*) m_cFlatSplitter.GetPane(0,1);

    ASSERT( m_pRightView );
    if( m_pRightView ){
        m_pRightView->FromDoc( m_pDoc );
    }

    return TRUE;
}

void CExportSplitterFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);

    if(IsWindow(GetSafeHwnd()))
        SendMessage(WS_MAXIMIZE);
}


CDictTreeView* CExportSplitterFrame::GetLeftView(){
    return m_pLeftView;
}
CExportOptionsView* CExportSplitterFrame::GetRightView(){
    return m_pRightView;
}
