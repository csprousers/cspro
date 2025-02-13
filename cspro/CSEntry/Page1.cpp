// Page1.cpp : implementation file
//

#include "StdAfx.h"
#include "Page1.h"
#include "CaseView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPage1 property page

IMPLEMENT_DYNCREATE(CPage1, CPropertyPage)

CPage1::CPage1() : CPropertyPage(CPage1::IDD)
{
    //{{AFX_DATA_INIT(CPage1)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_pCaseView = NULL;
}

CPage1::~CPage1()
{
}

void CPage1::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPage1)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPage1, CPropertyPage)
    //{{AFX_MSG_MAP(CPage1)
    ON_WM_SIZE()
    ON_WM_SHOWWINDOW()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPage1 message handlers

bool CPage1::CreateCaseView(CCreateContext* pContext)
{
    if( !IsWindow(GetSafeHwnd())){
        return false;
    }

    CRuntimeClass*  pViewClass  = RUNTIME_CLASS(CCaseView);

    ASSERT(pViewClass && pContext);
    CWnd * pWnd =(CWnd*) pViewClass->CreateObject();
    if (!pWnd){
        return false;
    }


    CRect rect;
    GetClientRect(&rect);

    DWORD dwStyle=AFX_WS_DEFAULT_VIEW;
    dwStyle&=~WS_BORDER;

    if ( !pWnd->Create(NULL, NULL, dwStyle, rect, this, 13576, pContext)) {
            AfxMessageBox(_T("Warning: couldn't create client area for tab view\n"));
            // pWnd will be cleaned up by PostNcDestroy
        return false;
    }

    m_pCaseView = (CCaseView*) pWnd;

    return true;
}


CCaseView* CPage1::GetCaseView()
{
    return m_pCaseView;
}

void CPage1::OnSize(UINT nType, int cx, int cy)
{
    CPropertyPage::OnSize(nType, cx, cy);

    // TODO: Add your message handler code here
    if(m_pCaseView && IsWindow(m_pCaseView->GetSafeHwnd())){
        m_pCaseView->MoveWindow(0,0,cx,cy);
    }
}







void CPage1::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CPropertyPage::OnShowWindow(bShow, nStatus);

    // TODO: Add your message handler code here
    CWnd* pMainWnd = AfxGetMainWnd();
    if(!pMainWnd || !IsWindow(pMainWnd->GetSafeHwnd()) ){
        return;
    }

    pMainWnd->PostMessage(UWM::CaseTree::Page1ChangeShowStatus, 0, 0);
}
