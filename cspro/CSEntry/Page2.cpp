// Page2.cpp : implementation file
//

#include "StdAfx.h"
#include "Page2.h"
#include "MainFrm.h"
#include "Rundoc.h"
#include <zCaseTreeF/CRunAplE.h>
#include <zCaseTreeF/TreeCnst.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPage2 property page

IMPLEMENT_DYNCREATE(CPage2, CPropertyPage)

CPage2::CPage2() : CPropertyPage(CPage2::IDD)
{
        //{{AFX_DATA_INIT(CPage2)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

        m_pCapiRunAplEntry      = NULL;
        m_pCaseTree             = NULL;
        m_pMainFrame            = NULL;
        m_iPageWidth            = 0;
        m_iPageHeight           = 0;
        m_icx                   = 0;
        m_icy                   = 0;
}

CPage2::~CPage2()
{
    if(m_pCapiRunAplEntry){
            delete(m_pCapiRunAplEntry);
            m_pCapiRunAplEntry = NULL;
    }
    if(m_pCaseTree){
            delete(m_pCaseTree);
            m_pCaseTree = NULL;
    }
}

void CPage2::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CPage2)
                // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPage2, CPropertyPage)
    //{{AFX_MSG_MAP(CPage2)
    ON_WM_SIZE()
    ON_WM_SHOWWINDOW()
    ON_MESSAGE(UWM::CaseTree::GoTo, OnGivenGoTo)                                    // FABN Nov 6, 2002
    ON_MESSAGE(UWM::CaseTree::GoToNode, OnGoToNode)                                 // FABN Nov 8, 2002
    ON_MESSAGE(UWM::CaseTree::TreeItemWithNothingToDo, OnTreeItemWithNothingToDo)   // FABN Nov 8, 2002
    ON_MESSAGE(UWM::CaseTree::RestoreEntryRunViewFocus, OnRestoreEntryRunViewFocus) // FABN Nov 8, 2002
    ON_MESSAGE(UWM::CaseTree::DeleteCaseTree, OnDeleteCaseTree)                     // FABN Nov 8, 2002
    ON_MESSAGE(UWM::CaseTree::UnknownKey, OnUnknownKey)                             // FABN Nov 8, 2002
    ON_MESSAGE(UWM::CaseTree::RecalcLayout, OnRecalcLayout)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPage2 message handlers
LRESULT CPage2::OnRestoreEntryRunViewFocus(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if(!m_pMainFrame){
                return FALSE;
        }

    m_pMainFrame->SendMessage(UWM::CaseTree::TreeItemWithNothingToDo, 0, 0);

    return TRUE;
}

LRESULT CPage2::OnDeleteCaseTree(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_pMainFrame->PostMessage(UWM::CaseTree::DeleteCaseTree, 0, 0);
    return TRUE;
}

LRESULT CPage2::OnTreeItemWithNothingToDo(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
        if(!m_pMainFrame){
                return FALSE;
        }

        m_pMainFrame->PostMessage(UWM::CaseTree::TreeItemWithNothingToDo, 0, 0);

        return TRUE;
}

LRESULT CPage2::OnGivenGoTo(WPARAM wParam, LPARAM lParam)
{
        if(!m_pMainFrame){
                return FALSE;
        }

        m_pMainFrame->PostMessage(UWM::CaseTree::GoTo, wParam, lParam);
        return TRUE;
}

LRESULT CPage2::OnGoToNode(WPARAM wParam, LPARAM lParam)
{
        if(!m_pMainFrame){
                return FALSE;
        }

        m_pMainFrame->PostMessage(UWM::CaseTree::GoToNode, wParam, lParam);
        return TRUE;
}

LRESULT CPage2::OnUnknownKey(WPARAM wParam, LPARAM lParam)
{
    if(!m_pMainFrame){

        CArray<DWORD,DWORD>*  pArray = (CArray<DWORD,DWORD>*) wParam;
        ASSERT(pArray->GetSize()==6);
        pArray->RemoveAll();
        delete(pArray);
        pArray = NULL;

        return FALSE;
    }

    m_pMainFrame->PostMessage(UWM::CaseTree::UnknownKey, wParam, lParam);

    return TRUE;
}

LRESULT CPage2::OnRecalcLayout(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    AfxGetMainWnd()->SendMessage(UWM::CSEntry::RecalcLeftLayout);
    return TRUE;
}

void CPage2::OnSize(UINT nType, int cx, int cy)
{

        CPropertyPage::OnSize(nType, cx, cy);
    m_icx = cx;
    m_icy = cy;

        #ifdef _DEBUG
                TRACE(_T("CPage2::OnSize(%d,%d,%d)\n"),nType,cx,cy);
        #endif


        // TODO: Add your message handler code here
        m_iPageWidth  = cx;
        m_iPageHeight = cy;
        if(m_pCaseTree && IsWindow(m_pCaseTree->GetSafeHwnd())){
                m_pCaseTree->MoveWindow(0,0,cx,cy);
        }
}

CCaseTree*      CPage2::GetCaseTree()
{
        return m_pCaseTree;
}




bool CPage2::CreateCaseTree()
{

    if(!m_pMainFrame || !IsWindow( GetSafeHwnd() )){
        return false;
    }

    CEntryrunDoc* pDoc = (CEntryrunDoc*) m_pMainFrame->GetActiveDocument();
    if(!pDoc){
        return false;
    }

    if(m_pCaseTree){
        return false;
    }


    //step #1 : build the interface to engine needed by the case tree
    CNPifFile*      pPifFile        =   pDoc->GetPifFile();
    CRunAplEntry*   pRunAplEntry    =   pDoc->GetRunApl();
    CDEItemBase*    pCurItemBase    =   pRunAplEntry->GetCurItemBase();
    m_pCapiRunAplEntry = new CCapiRunAplEntry( pRunAplEntry, pPifFile, pCurItemBase );

    //step #2 : build the case tree
    CDEField*   pCurField   =   pCurItemBase ? (CDEField*)pCurItemBase : NULL;
    int         iCurOcc     =   pCurField ? pCurField->GetParent()->GetCurOccurrence() : -1;
    ASSERT( iCurOcc!=0 );
    // int         iNumNodes   =   m_pCapiRunAplEntry->GetNNodes();
    // int         iTypeOfTree =   iNumNodes==1 ? TREE_OF_VARS_VALUES : TREE_OF_NODES;
    // the above line resulted in the tree not being displayed for 2+ level applications
    int iTypeOfTree = TREE_OF_VARS_VALUES;

    m_pCaseTree  =  new CCaseTree(  -1, iTypeOfTree, false, pCurField, iCurOcc, AfxGetApp(), m_pCapiRunAplEntry, this );
    if(!m_pCaseTree || !IsWindow(m_pCaseTree->GetSafeHwnd())){
        return false;
    }

    m_pCaseTree->Show(0,0,m_iPageWidth,m_iPageHeight);

    return (m_pCaseTree!=NULL);
}

bool CPage2::DeleteCaseTree()
{
    if(m_pCaseTree){
        delete(m_pCaseTree);
        m_pCaseTree = NULL;
    }

    if(m_pCapiRunAplEntry){
        delete(m_pCapiRunAplEntry);
        m_pCapiRunAplEntry = NULL;
    }

    return true;
}

void CPage2::OnShowWindow(BOOL bShow, UINT nStatus)
{
        CPropertyPage::OnShowWindow(bShow, nStatus);

        // TODO: Add your message handler code here
        if(bShow){

                if(!m_pCaseTree){

                        CreateCaseTree();


                } else {

                        if(!m_pMainFrame){
                                return;
                        }

                        CEntryrunDoc* pRunDoc = (CEntryrunDoc*) m_pMainFrame->GetActiveDocument();

                        CDEItemBase* pCurItem   = pRunDoc->GetCurField();
                        CDEField *       pCurField      = pCurItem ? (CDEField*)pCurItem : NULL;
                        int                      iCurOcc        = pCurField ? pCurField->GetParent()->GetCurOccurrence() : -1;

                        CMsgParam * pMsgParam   = new CMsgParam();
                        pMsgParam->dwArrayParam.Add( (DWORD) pCurField );
                        pMsgParam->iParam               = iCurOcc;
                        pMsgParam->bMustBeDestroyedAfterLastCatchMessage = true;

                        m_pMainFrame->PostMessage(UWM::CaseTree::RefreshCaseTree, (WPARAM)pMsgParam, 0);
                }
        }
}


void CPage2::SetMainFrame( CMainFrame* pMainFrame )
{
    m_pMainFrame = pMainFrame;
}


void CPage2::DummyMove()
{
    CCaseTree* pTree = GetCaseTree();
    if(!pTree){
        return;
    }

    pTree->SendMessage(UWM::CaseTree::ShowWindow, false);

        int dummyPix = 1;
        MoveWindow(4,22,m_icx,m_icy -dummyPix);
        MoveWindow(4,22,m_icx,m_icy +dummyPix);

    pTree->SendMessage(UWM::CaseTree::ShowWindow, true);
}
