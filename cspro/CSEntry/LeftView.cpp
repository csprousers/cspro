// LeftView.cpp : implementation file
//

#include "StdAfx.h"
#include "LeftView.h"
#include "CaseView.h"
#include "leftprop.h"
#include "MainFrm.h"
#include "Rundoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CFormView)

CLeftView::CLeftView()
        : CFormView(CLeftView::IDD)
{
        //{{AFX_DATA_INIT(CLeftView)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

        m_pPropSheet = NULL;
}

CLeftView::~CLeftView()
{
        if(m_pPropSheet){
                delete(m_pPropSheet);
                m_pPropSheet = NULL;
        }
}

void CLeftView::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CLeftView)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLeftView, CFormView)
        //{{AFX_MSG_MAP(CLeftView)
        ON_WM_SIZE()
        ON_WM_DESTROY()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
        CFormView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftView message handlers

bool CLeftView::CreatePropSheet(CCreateContext* pContext, CMainFrame* pMainFrame)
{
    if(m_pPropSheet)
        return false;

    m_pPropSheet = new CLeftPropSheet(_T(""),   /*empty caption*/
                                        this  /*the CFormView will be the parent*/,
                                        0             /*the first tab will be the default selected*/ );

    if( ! (m_pPropSheet->Create( this, WS_CHILD|WS_VISIBLE|WS_EX_CONTROLPARENT )!=0) )
        return false;

    m_pPropSheet->SetMainFrame(pMainFrame);

    return m_pPropSheet->CreateCaseView(pContext);
}

bool CLeftView::CreateCaseTree()
{
    ASSERT(m_pPropSheet);
    CEntryrunDoc*   pDoc  = (CEntryrunDoc*)((CMainFrame*)AfxGetMainWnd())->GetActiveDocument();
    bool  bShowCaseTree = pDoc->GetPifFile()->GetApplication()->GetShowCaseTree();
    if(!bShowCaseTree)
        return true;

    return m_pPropSheet->CreateCaseTree();
}

void CLeftView::OnSize(UINT nType, int cx, int cy)
{
        CFormView::OnSize(nType, cx, cy);
    m_icx = cx;
    m_icy = cy;

        #ifdef _DEBUG
                TRACE(_T("CLeftView::OnSize(%d,%d,%d)\n"),nType,cx,cy);
        #endif

        // TODO: Add your message handler code here
        if(m_pPropSheet && IsWindow(m_pPropSheet->GetSafeHwnd())){
                m_pPropSheet->MoveWindow(0,0,cx,cy);
        }
}

void CLeftView::OnDestroy()
{
        CFormView::OnDestroy();

        // TODO: Add your message handler code here
        if(!m_pPropSheet){
                return;
        }

        m_pPropSheet->DestroyWindow();
}

CCaseView* CLeftView::GetCaseView()
{
        return m_pPropSheet ? m_pPropSheet->GetCaseView() : NULL;
}

CCaseTree* CLeftView::GetCaseTree()
{
        return m_pPropSheet ? m_pPropSheet->GetCaseTree() : NULL;
}

void CLeftView::SetMainFrame(CMainFrame* pMainFrame)
{
        ASSERT(m_pPropSheet);
        m_pPropSheet->SetMainFrame(pMainFrame);
}

void CLeftView::DeleteCaseTree()
{
        ASSERT(m_pPropSheet);
        m_pPropSheet->DeleteCaseTree();
}

CLeftPropSheet* CLeftView::GetPropSheet()
{
        return m_pPropSheet;
}

void CLeftView::DummyMove()
{
    if(m_pPropSheet){
        m_pPropSheet->DummyMove();
    }
}
