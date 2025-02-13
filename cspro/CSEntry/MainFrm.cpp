// MainFrm.cpp : implementation of the CMainFrame class
#include "StdAfx.h"
#include "MainFrm.h"
#include "CSEntry.h"
#include "CaseView.h"
#include "CustMsg.h"
#include "DEBaseEdit.h"
#include "DiscDlg.h"
#include "DiscPDlg.h"
#include "DiscMDlg.h"
#include "DisPMDlg.h"
#include "GoToDlg.h"
#include "IntEDlg.h"
#include "leftprop.h"
#include "LeftView.h"   //FABN Nov 5, 2002
#include "ProgressDialog.h"
#include "OperatorStatistics.h"
#include "OperatorStatisticsLog.h"
#include "QuestionnaireSearchDlg.h"
#include "RunView.h"
#include "Rundoc.h"
#include "StatDlg.h"
#include <zToolsO/Utf8Convert.h>
#include <zToolsO/UWM.h>
#include <zUtilO/ArrUtil.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilF/MsgDial.h>
#include <zUtilF/SettingsDlg.h>
#include <zUtilF/UIThreadRunner.h>
#include <zHtml/UWM.h>
#include <zCaseO/Case.h>
#include <zDataO/CaseIterator.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zCapiO/QSFView.h>
#include <zCapiO/SelectDlg.h>
#include <zCaseTreeF/MsgParam.h>
#include <zCaseTreeF/CEUtils.h>
#include <engine/EngineObjectTransporter.h>
#include <engine/GPSThreadInfo.h>
#include <engine/flddef.h>
#include <zEngineO/Userbar.h>
#include <regex>


#ifdef _DEBUG
#define new DEBUG_NEW
    #undef THIS_FILE
    static char THIS_FILE[]= __FILE__;
#endif



static bool bShowIntDlg = true;
static CIntEdtDlg intEdtDlg;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)


BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)

    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_UPDATE_COMMAND_UI(ID_STOP, OnUpdateStop)
    ON_WM_ACTIVATE()
    ON_COMMAND(ID_STATS, OnStats)
    ON_UPDATE_COMMAND_UI(ID_STATS, OnUpdateStats)
    ON_COMMAND(ID_SAVE, OnPartialSaveCase)
    ON_UPDATE_COMMAND_UI(ID_SAVE, OnUpdatePartialSaveCase)
    ON_COMMAND(ID_FILE_SYNCHRONIZE, OnSynchronize)
    ON_UPDATE_COMMAND_UI(ID_FILE_SYNCHRONIZE, OnUpdateSynchronize)
    ON_COMMAND(ID_FILE_SETTINGS, OnCSProSettings)
    ON_WM_CLOSE()
    ON_COMMAND(ID_ADD, OnAddCase)
    ON_COMMAND(ID_MODIFY, OnModifyCase)
    ON_COMMAND(ID_PAUSE, OnPause)
    ON_UPDATE_COMMAND_UI(ID_ADD, OnUpdateAdd)
    ON_UPDATE_COMMAND_UI(ID_MODIFY, OnUpdateModify)
    ON_UPDATE_COMMAND_UI(ID_PAUSE, OnUpdatePause)
    ON_COMMAND(ID_PREV_CASE, OnPrevCase)
    ON_UPDATE_COMMAND_UI(ID_PREV_CASE, OnUpdatePrevCase)
    ON_COMMAND(ID_NEXT_CASE, OnNextCase)
    ON_UPDATE_COMMAND_UI(ID_NEXT_CASE, OnUpdateNextCase)
    ON_COMMAND(ID_FIRST_CASE, OnFirstCase)
    ON_UPDATE_COMMAND_UI(ID_FIRST_CASE, OnUpdateFirstCase)
    ON_COMMAND(ID_LAST_CASE, OnLastCase)
    ON_UPDATE_COMMAND_UI(ID_LAST_CASE, OnUpdateLastCase)
    ON_COMMAND(ID_NEXT_GROUP_OCC, OnNextGroupOcc)
    ON_UPDATE_COMMAND_UI(ID_NEXT_GROUP_OCC, OnUpdateNextGroupOcc)
    ON_COMMAND(ID_NEXT_GROUP, OnNextGroup)
    ON_UPDATE_COMMAND_UI(ID_NEXT_GROUP, OnUpdateNextGroup)
    ON_COMMAND(ID_NEXT_LEVEL, OnNextLevel)
    ON_UPDATE_COMMAND_UI(ID_NEXT_LEVEL, OnUpdateNextLevel)
    ON_COMMAND(ID_NEXT_LEVEL_OCC, OnNextLevelOcc)
    ON_UPDATE_COMMAND_UI(ID_NEXT_LEVEL_OCC, OnUpdateNextLevelOcc)
    ON_COMMAND(ID_PREV_SCREEN, OnPrevScreen)
    ON_UPDATE_COMMAND_UI(ID_PREV_SCREEN, OnUpdatePrevScreen)
    ON_COMMAND(ID_NEXT_SCREEN, OnNextScreen)
    ON_UPDATE_COMMAND_UI(ID_NEXT_SCREEN, OnUpdateNextScreen)
    ON_COMMAND(ID_DELETECASE, OnDeletecase)
    ON_UPDATE_COMMAND_UI(ID_DELETECASE, OnUpdateDeletecase)
    ON_COMMAND(ID_INSERT_GROUPOCC, OnInsertGroupocc)
    ON_UPDATE_COMMAND_UI(ID_INSERT_GROUPOCC, OnUpdateInsertGroupocc)
    ON_COMMAND(ID_DELETE_GRPOCC, OnDeleteGrpocc)
    ON_UPDATE_COMMAND_UI(ID_DELETE_GRPOCC, OnUpdateDeleteGrpocc)
    ON_COMMAND(ID_SORTGRPOCC, OnSortgrpocc)
    ON_UPDATE_COMMAND_UI(ID_SORTGRPOCC, OnUpdateSortgrpocc)
    ON_COMMAND(ID_INSERT_CASE, OnInsertCase)
    ON_UPDATE_COMMAND_UI(ID_INSERT_CASE, OnUpdateInsertCase)
    ON_COMMAND(ID_INSERT_GRPOCC_AFTER, OnInsertAfterOcc)
    ON_UPDATE_COMMAND_UI(ID_INSERT_GRPOCC_AFTER, OnUpdateInsertGrpoccAfter)
    ON_COMMAND(ID_FINDCASE, OnFindcase)
    ON_UPDATE_COMMAND_UI(ID_FINDCASE, OnUpdateFindcase)
    ON_COMMAND(ID_PREVIOUS_PERSISTENT, OnPreviousPersistent)
    ON_UPDATE_COMMAND_UI(ID_PREVIOUS_PERSISTENT, OnUpdatePreviousPersistent)
    ON_COMMAND(ID_VERIFY, OnVerifyCase)
    ON_UPDATE_COMMAND_UI(ID_VERIFY, OnUpdateVerify)
    ON_COMMAND(ID_GOTO, OnGoto)
    ON_UPDATE_COMMAND_UI(ID_GOTO, OnUpdateGoto)
    ON_WM_MENUCHAR()
    ON_COMMAND(ID_ADVTOEND, OnAdvtoend)
    ON_UPDATE_COMMAND_UI(ID_ADVTOEND, OnUpdateAdvtoend)
    ON_COMMAND(ID_INSERTNODE, OnInsertNode)
    ON_UPDATE_COMMAND_UI(ID_INSERTNODE, OnUpdateAddInsertNode)
    ON_COMMAND(ID_ADDNODE, OnAddNode)
    ON_UPDATE_COMMAND_UI(ID_ADDNODE, OnUpdateAddInsertNode)
    ON_COMMAND(ID_DELETENODE, OnDeleteNode)
    ON_UPDATE_COMMAND_UI(ID_DELETENODE, OnUpdateDeleteNode)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_PARTIALS, OnUpdatePartialsInd)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FIELD, OnUpdateFieldInd)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCC, OnUpdateOccInd)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_MODE, OnUpdateModeInd)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_SHOW, OnUpdateShowInd)
    ON_COMMAND(ID_FULLSCREEN, OnFullscreen)
    ON_UPDATE_COMMAND_UI(ID_FULLSCREEN, OnUpdateFullscreen)
    ON_COMMAND(ID_SHOW_CASE_LABELS, OnShowCaseLabels)
    ON_UPDATE_COMMAND_UI(ID_SHOW_CASE_LABELS, OnUpdateShowCaseLabels)
    ON_COMMAND(ID_TOGGLE_NAMES, OnToggleNames)
    ON_UPDATE_COMMAND_UI(ID_TOGGLE_NAMES, OnUpdateToggleNames)
    ON_COMMAND(ID_SORTORDER, OnSortorder)
    ON_UPDATE_COMMAND_UI(ID_SORTORDER, OnUpdateSortorder)
    ON_COMMAND(ID_INTEDITOPTIONS, OnInteractiveEditOptions)
    ON_UPDATE_COMMAND_UI(ID_INTEDITOPTIONS, OnUpdateInteractiveEditOptions)
    ON_COMMAND(ID_INTEDIT, OnInteractiveEdit)
    ON_UPDATE_COMMAND_UI(ID_INTEDIT, OnUpdateInteractiveEdit)
    ON_COMMAND(ID_LANGUAGE, OnLanguage)
    ON_UPDATE_COMMAND_UI(ID_LANGUAGE, OnUpdateLanguage)
    ON_COMMAND(ID_CAPITOGGLE, OnCapiToggle)
    ON_UPDATE_COMMAND_UI(ID_CAPITOGGLE, OnUpdateCapiToggle)
    ON_COMMAND(ID_CAPITOGGLEALLVARS, OnCapiToggleAllVars)
    ON_UPDATE_COMMAND_UI(ID_CAPITOGGLEALLVARS, OnUpdateCapiToggleAllVars)
    ON_COMMAND(ID_VIEW_REFUSALS, OnViewRefusals)
    ON_UPDATE_COMMAND_UI(ID_VIEW_REFUSALS, OnUpdateViewRefusals)
    ON_COMMAND(ID_VIEW_CHEAT, OnViewCheat)
    ON_UPDATE_COMMAND_UI(ID_VIEW_CHEAT, OnUpdateViewCheat)
    ON_COMMAND(ID_VIEW_ALL_CASES, OnViewAll)
    ON_COMMAND(ID_VIEW_NOT_DELETED_CASES_ONLY, OnViewNotDeleted)
    ON_COMMAND(ID_VIEW_DUPLICATE_CASES_ONLY, OnViewDuplicate)
    ON_COMMAND(ID_VIEW_PARTIAL_CASES_ONLY, OnViewPartial)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ALL_CASES, OnUpdateViewAll)
    ON_UPDATE_COMMAND_UI(ID_VIEW_NOT_DELETED_CASES_ONLY, OnUpdateViewNotDeleted)
    ON_UPDATE_COMMAND_UI(ID_VIEW_DUPLICATE_CASES_ONLY, OnUpdateViewDuplicate)
    ON_UPDATE_COMMAND_UI(ID_VIEW_PARTIAL_CASES_ONLY, OnUpdateViewPartial)
    ON_COMMAND(ID_EDIT_FIELD_NOTE, OnFieldNote)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FIELD_NOTE, OnUpdateNote)
    ON_COMMAND(ID_EDIT_CASE_NOTE, OnCaseNote)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CASE_NOTE, OnUpdateNote)
    ON_COMMAND(ID_EDIT_REVIEW_NOTES, OnReviewNotes)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REVIEW_NOTES, OnUpdateNote)
    ON_COMMAND(ID_STOP, OnStop)
    ON_COMMAND(ID_VIEW_QUESTIONNAIRE, OnViewQuestionnaire)
    ON_UPDATE_COMMAND_UI(ID_VIEW_QUESTIONNAIRE, OnUpdateViewQuestionnaire)

    //}}AFX_MSG_MAP
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)

    ON_MESSAGE(UWM::ToolsO::GetObjectTransporter, OnGetObjectTransporter)

    ON_MESSAGE(WM_IMSA_KEY_CHANGED, OnKeyChanged)
    ON_MESSAGE(WM_IMSA_WRITECASE, OnWriteCase) // RHF Mar 30, 2000
    ON_MESSAGE(WM_IMSA_ENGINEABORT, OnEngineAbort) // RHF Apr 25, 2000
    ON_MESSAGE(UWM::CSEntry::PreprocessEngineMessage, OnPreprocessEngineMessage)
    ON_MESSAGE(WM_IMSA_ENGINEMSG, OnEngineMessage) // RHF Jan 03, 2001
    ON_MESSAGE(WM_IMSA_ENTRY_TREEVIEW, IsUniqNames) // smg, jun 12, 2003
    ON_MESSAGE(WM_IMSA_SETSEQUENTIAL, OnSetSequential)
    ON_MESSAGE(WM_IMSA_SETCAPITEXT, OnSetCapiText)
    ON_MESSAGE(UWM::Capi::GetWindowHeight, OnGetWindowHeight)
    ON_MESSAGE(UWM::Capi::SetWindowHeight, OnSetWindowHeight)

    ON_MESSAGE(WM_IMSA_CSENTRY_REFRESH_DATA, OnEngineRefresh) // RHF Nov 19, 2001
    ON_MESSAGE(UWM::CSEntry::ShowCapi, OnEngineShowCapi) // RHF Nov 22, 2002
    ON_MESSAGE(UWM::CSEntry::RefreshSelected, OnRefreshSelected)

    ON_MESSAGE(WM_IMSA_USERBAR_UPDATE, OnUserbarUpdate)
    ON_MESSAGE(WM_IMSA_SET_MESSAGE_OVERRIDES, OnSetMessageOverrides) // 2010518
    ON_MESSAGE(UWM::CSEntry::UsingOperatorControlledMessages, OnUsingOperatorControlledMessages)
    ON_MESSAGE(WM_IMSA_GET_USER_FONTS, OnGetUserFonts) // 20100621
    ON_MESSAGE(WM_IMSA_GPS_DIALOG,OnShowGPSDialog) // 20100524

    ON_MESSAGE( WM_IMSA_REFRESHCAPIGROUPS, OnSelectiveRefreshCaseTree  )

    ON_MESSAGE( WM_IMSA_FIELD_BEHAVIOR,  OnFieldBehavior  )
    ON_MESSAGE( WM_IMSA_FIELD_VISIBILITY,  OnFieldVisibility  )

    ON_MESSAGE(WM_IMSA_CONTROL_PARADATA_KEYING_INSTANCE, OnControlParadataKeyingInstance)

    ON_MESSAGE(UWM::CaseTree::GoTo, OnGivenGoTo)                                    // FABN Nov  4, 2002
    ON_MESSAGE(UWM::CaseTree::RefreshCaseTree, OnRefreshCaseTree)                   // FABN Nov  6, 2002
    ON_MESSAGE(UWM::CaseTree::DeleteCaseTree, OnDeleteCaseTree)                     // FABN Nov  7, 2002
    ON_MESSAGE(UWM::CaseTree::CreateCaseTree, OnCreateCaseTree)                     // RHF Dec 19, 2002
    ON_MESSAGE(UWM::CaseTree::GoToNode, OnGoToNode)                                 // FABN Nov  8, 2002
    ON_MESSAGE(UWM::CaseTree::Page1ChangeShowStatus, OnPage1ChangeShowStatus)       // FABN Nov 11, 2002
    ON_MESSAGE(UWM::CaseTree::RestoreEntryRunViewFocus, OnRestoreEntryRunViewFocus) // FABN Nov 11, 2002
    ON_MESSAGE(UWM::CaseTree::TreeItemWithNothingToDo, OnTreeItemWithNothingToDo)   // FABN Nov 11, 2002
    ON_MESSAGE(UWM::CaseTree::UnknownKey, OnUnknownKey)                             // FABN Nov 20, 2002

    ON_COMMAND          (ID_TOGGLE_CASE_TREE, OnToggleCaseTree)                     // FABN Jan 16, 2003
    ON_UPDATE_COMMAND_UI(ID_TOGGLE_CASE_TREE, OnUpdateToggleCaseTree)               // FABN Jan 16, 2003

    ON_MESSAGE(UWM::CSEntry::AcceleratorKey, OnAcceleratorKey)                      // FABN Jan 15, 2003
    ON_MESSAGE(UWM::CSEntry::RecalcLeftLayout, OnRecalcLeftLayout)
    ON_MESSAGE(UWM::CSEntry::CaseTreeFocus, OnCasesTreeFocus)                       // FABN March 3, 2003

    ON_MESSAGE(WM_IMSA_PARTIAL_SAVE, OnPartialSaveFromApp)

    ON_MESSAGE(WM_IMSA_PROGRESS_DIALOG_SHOW, OnShowProgressDialog)
    ON_MESSAGE(WM_IMSA_PROGRESS_DIALOG_HIDE, OnHideProgressDialog)
    ON_MESSAGE(WM_IMSA_PROGRESS_DIALOG_UPDATE, OnUpdateProgressDialog)

    ON_MESSAGE(WM_IMSA_CHANGE_INPUT_REPOSITORY, OnChangeInputRepository)
    ON_MESSAGE(WM_IMSA_WINDOW_TITLE_QUERY, OnWindowTitleQuery)

    ON_MESSAGE(WM_IMSA_PORTABLE_ENGINEUI, OnEngineUI)
    ON_MESSAGE(UWM::UtilF::RunOnUIThread, OnRunOnUIThread)
    ON_MESSAGE(UWM::UtilF::GetApplicationShutdownRunner, OnGetApplicationShutdownRunner)
    ON_MESSAGE(UWM::Html::ActionInvokerEngineProgramControlExecuted, OnActionInvokerEngineProgramControlExecuted)

END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_PARTIALS,  // smg: 9 apr 2003; do partials exist in the datafile?
    ID_INDICATOR_MODE,
    ID_INDICATOR_SHOW,   // BMD 05 Mar 2002
    ID_INDICATOR_FIELD,
    ID_INDICATOR_OCC,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    m_bPause = false;
    m_bClose = TRUE;
    m_eScreenView = FULLSCREEN_NO;
    m_bShowCaseLabels = true;
    m_bViewNames = true;
    m_bPartialSave = false;
    m_iWidth =0;
    m_pLeftView = NULL;     //FABN Nov 6, 2002
//    m_bTreeEnable = true;
    m_bCaseTreeActiveOnStart = true;
    m_eRepoType = DataRepositoryType::Null;
    m_eCaseStatusToShow = CaseIterationCaseStatus::NotDeletedOnly;

    //FABN Jan 15, 2003
    bool bCtrl  = true;
    bool bShift = true;
    bool bAlt   = true;
    AddToAccelArrays( ID_ADD,               _T('A')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays(ID_VIEW_PARTIAL_CASES_ONLY,    _T('B')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_CAPITOGGLE,        _T('C')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_OPENDAT_FILE,      _T('D')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_EDIT_FIELD_NOTE,   _T('N')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_FINDCASE,          _T('F')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_CAPITOGGLEALLVARS, _T('K')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_LANGUAGE,          _T('L')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_MODIFY,            _T('M')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_FILE_OPEN,         _T('O')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_PAUSE,             _T('P')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_SORTORDER,         _T('Q')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_STOP,              _T('S')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_FILE_SYNCHRONIZE,  _T('S')     , bCtrl,  bShift, !bAlt);
    AddToAccelArrays( ID_SAVE,              _T('R')     ,!bCtrl, !bShift, bAlt );
    AddToAccelArrays( ID_TOGGLE_NAMES,      _T('T')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_FULLSCREEN,        _T('J')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_VERIFY,            _T('V')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_STATS,             _T('W')     , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_TOGGLE_CASE_TREE,  _T('Z')     , bCtrl, !bShift, !bAlt);
    AddToAccelArrays( ID_ADVTOEND,          VK_F10      , !bCtrl,!bShift, !bAlt);
    AddToAccelArrays( ID_GOTO,              VK_F6       , !bCtrl,!bShift, !bAlt); // RHF Sep 20, 2007
    AddToAccelArrays( ID_INTEDIT,           VK_F11      , !bCtrl,!bShift, !bAlt);
    AddToAccelArrays( ID_VIEW_CHEAT,        VK_F2       , bCtrl, !bShift, !bAlt );
    AddToAccelArrays( ID_HELP_FINDER,       VK_F1       , !bCtrl,!bShift, !bAlt);


    m_iNumCases = 0;
    m_iTotalPartial = 0;
    m_iNumPartialAdd = 0;
    m_iNumPartialModify = 0;
    m_iVerified = 0;
    m_iPartialVerify = 0;

    m_bPartialSaveFromApp = false; // RHF Feb 23, 2004
    m_bPartialSaveClearSkipped = false;
    m_bOnStop = false ; //vc 7 compatibilty

    m_bCaseTreeSortedOrder = false;

    m_bPage1StatusChangeStopFocusChangeHack = false;

    m_pProgressDlg = NULL;
}

CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
        //      if (!m_wndToolBar.CreateEx(this,TBSTYLE_FLAT,WS_CHILD | WS_VISIBLE | CBRS_TOP,CRect(0,0,0,0), AFX_IDW_TOOLBAR ) ||
        //              !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }
    /*if (!m_wndDlgBar.Create(this, IDR_MAINFRAME,
    CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
    {
        TRACE0(_T("Failed to create dialogbar\n"));
    return -1;              // fail to create
}*/
    if (!m_wndReBar.Create(this) ||
    !m_wndReBar.AddBar(&m_wndToolBar) /*||
    !m_wndReBar.AddBar(&m_wndDlgBar)*/)
    {
        TRACE0("Failed to create rebar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
        sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    // TODO: Remove this if you don't want tool tips
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |CBRS_BORDER_3D|
        CBRS_TOOLTIPS | CBRS_FLYBY);

    return 0;
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs
    static BOOL bFirst = TRUE;
    if(bFirst) {
        WNDCLASS wndcls;
        ::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wndcls);
        CIMSAString csClassName = CSPRO_WNDCLASS_ENTRYFRM;
        wndcls.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
        wndcls.lpszClassName = csClassName;
        VERIFY(AfxRegisterClass(&wndcls));
        cs.lpszClass = csClassName;
        bFirst = FALSE;
    }
    cs.lpszClass = CSPRO_WNDCLASS_ENTRYFRM ;


    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
CEntryrunDoc* CMainFrame::GetDocument()
{
    return assert_cast<CEntryrunDoc*>(GetActiveDocument());
}


LRESULT CMainFrame::OnGetObjectTransporter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if( m_objectTransporter == nullptr )
    {
        CEntryrunDoc* pDoc = GetDocument();
        CRunAplEntry* pRunApl = ( pDoc != nullptr ) ? pDoc->GetRunApl() : nullptr;

        if( pRunApl != nullptr )
            m_objectTransporter = std::make_unique<EngineObjectTransporter>(&pRunApl->GetEntryDriver()->m_EngineArea);
    }

    return reinterpret_cast<LRESULT>(m_objectTransporter.get());
}


void CMainFrame::OnUpdateStop(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pDoc->GetAppMode() == MODIFY_MODE || pDoc->GetAppMode() == ADD_MODE || pDoc->GetAppMode() == VERIFY_MODE)

        pCmdUI->Enable( TRUE );
    else
        pCmdUI->Enable( FALSE);
}

/*void CMainFrame::UpdateDlgBar()
{
//      m_wndDlgBar.m_iMode = -1;
//      m_wndDlgBar.UpdateData(FALSE);
}*/

void CMainFrame::ActivateFrame(int nCmdShow)
{
    CFrameWnd::ActivateFrame(nCmdShow);
}

void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
    CFrameWnd::OnActivate(nState, pWndOther, bMinimized);

    //Set the focus to the current field in the entry
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    if(!pDoc)
        return;

    CRunAplEntry* pApl = pDoc->GetRunApl();
    APP_MODE mode = pDoc->GetAppMode();
    if(pApl && pApl->HasAppLoaded() && ( (mode == MODIFY_MODE) || (mode ==ADD_MODE))) {
            CView* pView = GetRunView();


        if(pView && pView->IsKindOf(RUNTIME_CLASS(CEntryrunView))) {
            //Set this activation of cursor SAVY &&&
            CEntryrunDoc* pViewDoc =  (CEntryrunDoc*)((CEntryrunView*)pView)->GetDocument();
            if(pViewDoc->GetCurField()) {
                CDEBaseEdit* pEdit = ((CEntryrunView*)pView)->SearchEdit((CDEField*)pViewDoc->GetCurField());
                if(pEdit)
                    pEdit->SetFocus();
            }
        }
    }


}


BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    BOOL bRet = FALSE;
    // TODO: Add your specialized code here and/or call the base class
    UNREFERENCED_PARAMETER(lpcs);
    //      return CFrameWnd::OnCreateClient(lpcs, pContext);

    // Create splitter window
    VERIFY(m_wndSplitter.CreateStatic(this,1,2));

    VERIFY(m_wndCapiSplitter.CreateStatic(&m_wndSplitter,2,1,WS_CHILD | WS_VISIBLE | WS_BORDER,m_wndSplitter.IdFromRowCol(0, 1)));

    //FABN Nov 5, 2002
    bool  bRightViewOK = m_wndCapiSplitter.CreateView(0,0,RUNTIME_CLASS(QSFView), CSize(120,120), pContext) != 0;

    bRightViewOK = m_wndCapiSplitter.CreateView(1,0,RUNTIME_CLASS(CEntryrunView), CSize(120,120), pContext) != 0;

    //VERIFY(m_wndSplitter.CreateView(0,1,RUNTIME_CLASS(CEntryrunView), CSize(120,120), pContext));

    //FABN Nov 5, 2002
    //VERIFY(m_wndSplitter.CreateView(0,0,RUNTIME_CLASS(CCaseView), CSize(120,120), pContext));
    bool bLeftViewOK = bRightViewOK ? m_wndSplitter.CreateView(0,0,RUNTIME_CLASS(CLeftView), CSize(120,120), pContext) != 0 : false;

    if(!bRightViewOK || !bLeftViewOK ){
        m_wndSplitter.DestroyWindow();
        return FALSE;
    }

    if(bLeftViewOK){
        m_pLeftView = (CLeftView*) m_wndSplitter.GetPane(0,0);

                if( m_pLeftView )
                        m_pLeftView->CreatePropSheet(pContext, this);

        CRect r;
        GetClientRect(&r);

        int w1 = (int)(0.25*r.Width());
        int w2 = (int)(0.75*r.Width());

        m_wndSplitter.SetColumnInfo( 0, w1, 0 );
        m_wndSplitter.SetColumnInfo( 1, w2, 0 );
        m_wndSplitter.RecalcLayout();

        bRet = TRUE;
    }

    return bRet;
}



CCaseView* CMainFrame::GetCaseView()    //FABN Nov 5, 2002
{
    return m_pLeftView ? m_pLeftView->GetCaseView() : NULL;
}

CCaseTree* CMainFrame::GetCaseTree()    //FABN Nov 6, 2002
{
    return m_pLeftView ? m_pLeftView->GetCaseTree() : NULL;
}


void CMainFrame::OnStats()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    COperatorStatisticsLog* pOperatorStatisticsLog = NULL;

    if( pDoc != NULL )
        pOperatorStatisticsLog = pDoc->GetOperatorStatisticsLog();

    if( pOperatorStatisticsLog == NULL || pOperatorStatisticsLog->GetOpStatsArray().GetSize() == 0 )
    {
        AfxMessageBox(_T("No operator statistics available."));
        return;
    }

    CStatDlg statDlg;

    statDlg.m_StatGrid.m_pOperatorStatisticsLog = pOperatorStatisticsLog;

    // update the number of cases and the number of verified cases
    statDlg.m_sNumCases = IntToString(m_iNumCases);
    statDlg.m_sVerifiedCases = IntToString(m_iVerified);

    if( m_iNumCases > 0 )
        statDlg.m_sVerifiedCases.AppendFormat(_T(" (%0.0f%%)"),(float)m_iVerified / m_iNumCases * 100);

    statDlg.DoModal();
}

void CMainFrame::OnUpdateStats(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pDoc && pDoc->GetPifFile() && pDoc->GetPifFile()->GetStatsLockFlag()){
        pCmdUI->Enable(FALSE);
        return;
    }
    if(pDoc && pDoc->GetAppMode() != NO_MODE){
        pCmdUI->Enable(FALSE);
        return;
    }
    pCmdUI->Enable(TRUE);

}

//////////////////////////////////////////////////////////////////////

LONG CMainFrame::IsUniqNames (WPARAM wParam, LPARAM lParam)
{
    return m_bViewNames ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////


void CMainFrame::DoNote(bool bCaseNote)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    if( pRunApl->EditNote(bCaseNote) )
        pRunDoc->SetQModified(TRUE);
}



/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnClose()
//
/////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnClose()
{
    if( !m_bClose )
        return;

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    if( pRunApl != nullptr )
    {
        if( pDoc->GetAppMode() != NO_MODE )
        {
            bool close_csentry_after_stopping = false;
            OnStop(&close_csentry_after_stopping);

            // if an OnStop callback leads to the stop being canceled, quit out
            if( !m_bOnStop )
                return;

            // if the case listing is locked, close CSEntry after the first interaction with a case
            if( close_csentry_after_stopping )
                return;
        }

        if( pRunApl->HasModifyStarted() )
            pRunApl->ModifyStop();

        pRunApl->Stop();
    }

    SendMessage(UWM::CaseTree::DeleteCaseTree, 0, 0);

    if( m_pProgressDlg != nullptr)
    {
        m_pProgressDlg->DestroyWindow();
        SAFE_DELETE(m_pProgressDlg);
    }

    CFrameWnd::OnClose();
}


/////////////////////////////////////////////////////////////////////////////////
//
//      adding, inserting, modifying, and verifying cases
//
/////////////////////////////////////////////////////////////////////////////////

bool CMainFrame::CanStopAddMode() // if they are in add mode, see if they want to stop
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    if( pDoc->GetAppMode() == ADD_MODE )
    {
        OnStop();

        if( !m_bOnStop )
            return false;
    }

    return true;
}

bool CMainFrame::PreCaseLoadingStartActions(APP_MODE appMode)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    m_bPause = false;

    // ignore if no application open
    if( pRunApl == NULL )
    {
        pDoc->SetAppMode(NO_MODE);
        UpdateToolBar();
        return false;
    }

    // ignore if already in this mode
    if( pDoc->GetAppMode() == appMode )
    {
        CEntryrunView* pView = GetRunView();
        pView->GoToFld((CDEField*)pDoc->GetCurField());
        UpdateToolBar();
        return false;
    }

    // try to stop the engine (user may be in another mode and not want to)
    OnStop();

    if( !m_bOnStop )
        return false;

    GetRunView()->ResetGrids();

    // start the engine
    int iRunAplMode =   ( appMode == ADD_MODE ) ? CRUNAPL_ADD :
                        ( appMode == MODIFY_MODE ) ? CRUNAPL_MODIFY : CRUNAPL_VERIFY;

    if( !pRunApl->Start(iRunAplMode) )
    {
        CString csMsg;
        csMsg.Format(_T("Cannot start %s."),(LPCTSTR)pDoc->GetPifFile()->GetEvaluatedAppDescription());
        AfxMessageBox( csMsg );

        pDoc->SetAppMode(NO_MODE);
        UpdateToolBar();
        pRunApl->ModifyStop();
        pRunApl->Stop();

        return false;
    }

    if( !pRunApl->HasStarted() )
        pRunApl->Start(iRunAplMode);

    return true;
}


bool CMainFrame::PostCaseLoadingStartActions(APP_MODE appMode,NODEINFO* pNodeInfo/* = nullptr*/,CRunAplEntry::ProcessModifyAction eModifyAction/* = CRunAplEntry::ProcessModifyAction::GotoNode*/)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    CDEItemBase* pItem = NULL;
    bool bMoved = true;
    auto partial_save_mode = PartialSaveMode::None;

    bool bLoadSuccess = false;

    if( appMode == ADD_MODE )
    {
        pItem = pRunApl->NextField(FALSE);
        bLoadSuccess = ( pItem != NULL ); // the application may not return a valid field
    }

    else if( ( appMode == MODIFY_MODE ) || ( appMode == VERIFY_MODE ) )
    {
        // only allow verification starting with the root level
        int iNode = ( appMode == VERIFY_MODE ) ? 0 : pNodeInfo->iNode;

        if( pRunApl->ModifyStart() && pRunApl->ProcessModify(pNodeInfo->case_summary.GetPositionInRepository(),&bMoved,&partial_save_mode,&pItem,iNode,eModifyAction) )
            bLoadSuccess = true;
    }

    if( !bLoadSuccess )
    {
        pDoc->SetAppMode(NO_MODE);
        UpdateToolBar();
        pRunApl->ModifyStop();
        pRunApl->Stop();
        OnStop();
        return false;
    }

    pDoc->SetAppMode(appMode);

    if( bMoved )
        pDoc->SetCurField((CDEField*)pItem);

    CEntryrunView* pView = GetRunView();

    SetRedraw(TRUE);

    if( partial_save_mode != PartialSaveMode::Verify )
    {
        pView->InvalidateRect(NULL);
        pView->UpdateWindow();
        pView->ResetForm();
    }

    GetCaseView()->SetRedraw(FALSE);
    GetCaseView()->UpdateInsertText();
    GetCaseView()->SetRedraw(TRUE);

    if( pDoc->GetCurField() != NULL )
        pView->ScrollToField((CDEField*)pDoc->GetCurField());

    else if( pDoc->GetAppMode() != ADD_MODE )
    {
        ASSERT(0); // .... REPO_TEMP what is the case where we would get here?
        pDoc->SetQModified(FALSE);
        pView->ProcessModifyMode();
        return false;
    }

    m_pLeftView->CreateCaseTree();

    SetActiveView(pView);

    CDEField* pCurField = (CDEField*)pDoc->GetCurField();
    CDEBaseEdit* pCurEdit = ( pCurField != NULL ) ? pView->SearchEdit(pCurField) : NULL;
    pView->SendMessage(UWM::CSEntry::ShowCapi, 0, (WPARAM)pCurEdit);

    if( pCurEdit != NULL )
        pCurEdit->PostMessage(WM_SETFOCUS);

    // potentially change the mode based on the partial save status
    if( partial_save_mode == PartialSaveMode::Modify )
        pDoc->SetAppMode(MODIFY_MODE);

    else if( partial_save_mode == PartialSaveMode::Add )
        pDoc->SetAppMode(ADD_MODE);

    else if( partial_save_mode == PartialSaveMode::Verify )
    {
        pDoc->SetAppMode(VERIFY_MODE);
        pView->InvalidateRect(NULL);
        pView->UpdateWindow();
        pView->ResetForm();
    }

    UpdateToolBar();

    return true;
}


void CMainFrame::OnAddCase()
{
    GetCaseView()->SetCaseNumber(-1);

    if( !PreCaseLoadingStartActions(ADD_MODE) )
        return;

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    pRunApl->ProcessAdd();

    PostCaseLoadingStartActions(ADD_MODE);
}


void CMainFrame::OnInsertCase()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    CCaseView* pCaseView = GetCaseView();
    CTreeCtrl& caseTree = pCaseView->GetTreeCtrl();
    HTREEITEM hSelItem = caseTree.GetSelectedItem();

    if( hSelItem == NULL )
        return;

    NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hSelItem);

    pCaseView->SetCaseNumber(pNodeInfo->case_number);

    if( !PreCaseLoadingStartActions(ADD_MODE) )
        return;

    pRunApl->ProcessInsert(pNodeInfo->case_summary.GetPositionInRepository());

    PostCaseLoadingStartActions(ADD_MODE);
}


void CMainFrame::OnModifyCase()
{
    if( !CanStopAddMode() )
        return;

    CCaseView* pCaseView = GetCaseView();
    CTreeCtrl& caseTree = pCaseView->GetTreeCtrl();

    // always select an item if one has not been selected before selecting modify mode
    HTREEITEM hSelItem = caseTree.GetSelectedItem();

    if( hSelItem == NULL )
    {
        hSelItem = caseTree.GetRootItem();

        if( hSelItem == NULL )
        {
            AfxMessageBox (_T("There are no cases in the data file so there is nothing to modify."));
            return;
        }

        caseTree.Select(hSelItem,TVGN_CARET);
    }

    NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hSelItem);
    ModifyStarterHelper(pNodeInfo,CRunAplEntry::ProcessModifyAction::GotoNode);
}


bool CMainFrame::ModifyStarterHelper(NODEINFO* pNodeInfo,CRunAplEntry::ProcessModifyAction eModifyAction)
{
    if( !PreCaseLoadingStartActions(MODIFY_MODE) )
        return false;

    CCaseView* pCaseView = GetCaseView();
    pCaseView->SetCaseNumber(pNodeInfo->case_number);

    return PostCaseLoadingStartActions(MODIFY_MODE,pNodeInfo,eModifyAction);
}


void CMainFrame::OnVerifyCase()
{
    if( !CanStopAddMode() )
        return;

    if( m_iNumPartialAdd > 0 || m_iNumPartialModify > 0 )
    {
        AfxMessageBox(_T("You have one or more partial cases in your data file. Please complete these before you start verification process."));
        return;
    }

    if( GetSortFlag() ) // if the frequency is not 1, don't allow the cases to be displayed in sorted order
    {
        CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
        Application* pApp = pDoc->GetPifFile()->GetApplication();

        if( pApp->GetVerifyFreq() != 1 )
        {
            AfxMessageBox(_T("You cannot use verify mode with a frequency other than 1 while viewing the cases in sort order."));
            return;
        }
    }

    if( !PreCaseLoadingStartActions(VERIFY_MODE) )
        return;

    // see if there is a valid case for verification
    if( !SelectNextCaseForVerification() )
    {
        CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
        CRunAplEntry* pRunApl = pDoc->GetRunApl();
        pRunApl->Stop();
        OnStop();
        return;
    }

    CCaseView* pCaseView = GetCaseView();
    CTreeCtrl& caseTree = pCaseView->GetTreeCtrl();
    HTREEITEM hSelItem = caseTree.GetSelectedItem();

    NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hSelItem);

    GetRunView()->ResetVerifyString();

    PostCaseLoadingStartActions(VERIFY_MODE,pNodeInfo);
}



/////////////////////////////////////////////////////////////////////////////////
//
//      BOOL CMainFrame::OnStop()
//
/////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnStop(bool* close_csentry_after_stopping)
{
    m_bOnStop = false;
    bool bSelModKey = false;
    bool bRestoreSelPos = false;
    m_bPause = false;
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();
    APP_MODE appMode = pDoc->GetAppMode();

    // ignore if no application open or if already in this mode
    if( pRunApl == NULL || appMode == NO_MODE )
    {
        pDoc->SetAppMode(NO_MODE);
        UpdateToolBar();
        m_bOnStop = true;
        return;
    }

    //Get Active Tab in the property sheet.  To get the case tree init  condition for the next engine start mode
    int iCurTabSel = m_pLeftView->GetPropSheet()->GetTabControl()->GetCurSel();
    if (appMode == ADD_MODE || appMode == MODIFY_MODE || appMode == VERIFY_MODE) {
   // RHF END  Dec 11, 2003 BUCEN_DEC2003 Changes
        bool    bStopFromApp=(pDoc->GetCurField()==NULL);// RHF Feb 23, 2004
        bool    bHasStopFunction=pRunApl->HasSpecialFunction(SpecialFunction::OnStop);
        bool    bCancelStop = !bStopFromApp && bHasStopFunction && (pRunApl->ExecSpecialFunction(SpecialFunction::OnStop) == 0); // RHF Feb 23, 2004 Add  !bStopFromApp

        if( !bStopFromApp && bHasStopFunction && pRunApl->HasSomeRequest() // RHF Feb 23, 2004 Add  !bStopFromApp
                                              && !pRunApl->IsEndingModifyMode() ) { // 20110222 added !IsEndingModifyMode
            bCancelStop = true;
            if( GetRunView() ) {
                GetRunView()->PostMessage(UWM::CSEntry::MoveToField, 0, 0);
                m_bOnStop = false;
                return;
            }
        }
        // RHF END  Dec 11, 2003 BUCEN_DEC2003 Changes
        if(appMode == MODIFY_MODE){
            bSelModKey = true;
        }
        else if (appMode == ADD_MODE){
            bRestoreSelPos = true;
        }

        if( pDoc->GetOperatorStatisticsLog() != NULL )
            pDoc->GetOperatorStatisticsLog()->Save();

        //fdef//VERSION_2_VC                    //      <begin> // victor Jan 20, 02
        if( pDoc && pRunApl != NULL ) {

            if(  pDoc->GetQModified() || bCancelStop  ) { // RHF Dec 12, 2003 BUCEN_DEC2003 Changes
                // smg: ideally here, we shld check if no IDs, or all IDs, have been keyed;
                // if they're in the middle of level 1 IDs, then the case is discarded;
                // if they're in the middle of level 2 or 3 IDs, then the case is saved,
                // but the current node is discarded (can the save func support this?)

                // replaced/augmented previous code block w/the following (smg, 3-31-03)

                CEntryrunView* pView = GetRunView();

                int ok2save = pRunApl->QidReady(pRunApl->GetCurrentLevel());
                bool bAllowPartial = pDoc->GetPifFile()->GetApplication()->GetPartialSave();
                bool bEnablePartialButton = ok2save && bAllowPartial;
                BOOL bPathOn = pDoc->GetCurFormFile()->IsPathOn();
                CString csFinishText = bPathOn ? _T("&Advance to End") : _T("&Finish");
                END_MODE eEndMode = CANCEL_MODE;

               // RHF INIT Dec 12, 2003 BUCEN_DEC2003 Changes
                if( bHasStopFunction ) {
                    eEndMode = bCancelStop ? CANCEL_MODE : DISCARD_CASE;
                }
                else {
                    // RHF END  Dec 12, 2003 BUCEN_DEC2003 Changes

                    switch(appMode) {
                    case ADD_MODE :
                        {
                            if (pRunApl->IsNewCase())       // smg: so esc works prop for new case!
                                eEndMode = DISCARD_CASE;
                            else if(bAllowPartial){
                                CDiscPDlg discPDlg;
                                discPDlg.m_bEnablePartialButton = bEnablePartialButton;
                                discPDlg.DoModal();
                                eEndMode = discPDlg.m_eMode;
                            }
                            else {
                                CDiscDlg discDlg;
                                discDlg.DoModal();
                                eEndMode = discDlg.m_eMode;
                            }
                        }
                        break;
                    case VERIFY_MODE:
                        {
                            if(bAllowPartial){
                                CDiscPDlg discPDlg;
                                discPDlg.m_eAppMode = VERIFY_MODE;
                                discPDlg.m_bEnablePartialButton = bEnablePartialButton;
                                discPDlg.DoModal();
                                eEndMode = discPDlg.m_eMode;
                            }
                            else {
                                CDiscDlg discDlg;
                                discDlg.m_eAppMode = VERIFY_MODE;
                                discDlg.DoModal();
                                eEndMode = discDlg.m_eMode;
                            }
                        }
                        break;
                    case MODIFY_MODE:
                        {
                            if(bAllowPartial){
                                CDisPMDlg discPMDlg;
                                discPMDlg.m_bEnablePartialButton = bEnablePartialButton;
                                discPMDlg.SetFinishText(csFinishText);
                                discPMDlg.DoModal();
                                eEndMode = discPMDlg.m_eMode;
                            }
                            else {
                                CDiscMDlg discMDlg;
                                discMDlg.SetFinishText(csFinishText);
                                discMDlg.DoModal();
                                eEndMode = discMDlg.m_eMode;
                            }
                        }
                        break;
                    default:
                        ASSERT(FALSE);
                        break;
                    }
                }         // RHF Dec 12, 2003 BUCEN_DEC2003 Changes
                switch(eEndMode){
                case PARTIAL_SAVE:
                    OnPartialSaveCase();
                    break;
                case FINISH_CASE:
                {
                    if( bPathOn )
                    {
                        OnAdvtoend();
                        return;
                    }
                    else
                        OnNextLevel();

                    break;
                }
                case DISCARD_CASE:
                    pView->RemoveAllGrids2(false);      // don't save, just bail
                    pRunApl->ResetDoorCondition();      // formally closing the CsDriver session
                    break;
                case CANCEL_MODE:
                    pView->GoToFld((CDEField*)pDoc->GetCurField()); // fuggedaboutit, go back2keying
                    UpdateToolBar();
                    m_bOnStop = false;
                    return;
                default:
                    m_bOnStop = false;
                    return;
                }
            }
        }
    }

     bool        bResetDoor=true; // RHF Jul 28, 2003
     if( bResetDoor || appMode == ADD_MODE ) // RHF Jul 28, 2003 Add bResetDoor
         pRunApl->ResetDoorCondition();
            // RHF END Jun 03, 2003

    if (pDoc->IsPartialAdd() || appMode == MODIFY_MODE || appMode == VERIFY_MODE) {
        // stop modify mode
        if(pRunApl->HasModifyStarted()){
            BOOL bRet = pRunApl->ModifyStop();
            if (!bRet) {
                AfxMessageBox (_T("Error in ModifyStop()"));
            }
        }
    }

    pDoc->SetQModified(FALSE);
    pDoc->SetAppMode(NO_MODE);

    bool bRet = pRunApl->Stop();

    CString csMsg;
    if( !bRet ) {
        csMsg.Format( _T("Error to do stop") );
        AfxMessageBox( csMsg );
    }

    SendMessage(UWM::CaseTree::DeleteCaseTree, 0, 0); //FABN Nov 7, 2002

    UpdateToolBar();

    if( pDoc->GetOperatorStatisticsLog() != NULL )
        pDoc->GetOperatorStatisticsLog()->StopStatsObj();

    CEntryrunView* pView = GetRunView();

    pView->ResetGrids();
    pView->InvalidateRect(NULL);
    pView->UpdateWindow();
    GetCaseView()->UpdateInsertText();

    pView->SetCheatKey(FALSE);
    bShowIntDlg = true;
    GetCaseView()->SetFocus();


    HTREEITEM  hModSelItem = NULL;
    CIMSAString sModSelKey;
    CTreeCtrl& caseTree = GetCaseView()->GetTreeCtrl();
    if(bSelModKey){
        hModSelItem = caseTree.GetSelectedItem();
        if(hModSelItem){
            sModSelKey = caseTree.GetItemText(hModSelItem);
            hModSelItem = NULL;
        }
    }

    // after the first interaction with a case, close CSEntry if the case listing is locked
    // or if the Key= parameter is at least the full length of a case ID
    CNPifFile* pPifFile = pDoc->GetPifFile();

    if( pPifFile->GetCaseListingLockFlag() || pPifFile->GetKey().GetLength() >= pRunApl->GetInputDictionaryKeyLength() )
    {
        if( close_csentry_after_stopping != nullptr )
            *close_csentry_after_stopping = true;

        AfxGetMainWnd()->SendMessage(WM_CLOSE);
        return;
    }


    //Savy added to set the partials right
    BuildKeyArray();
    GetCaseView()->BuildTree();


    if(bSelModKey && !sModSelKey.IsEmpty()){

        HTREEITEM hItem = GetCaseView()->GetTreeCtrl().GetRootItem();

        BOOL bFound = FALSE;
        while(hItem) {
            CString sString = GetCaseView()->GetTreeCtrl().GetItemText(hItem);
            if(sString.CompareNoCase(sModSelKey) ==0 ){
                GetCaseView()->GetTreeCtrl().Select(hItem,TVGN_CARET);
                bFound = TRUE;
                break;
            }
            hItem = GetCaseView()->GetTreeCtrl().GetNextSiblingItem(hItem);
        }
    }
    else if(bRestoreSelPos){
        GetCaseView()->RestoreSelectPos();
    }
    if(iCurTabSel == 0) {
        m_bCaseTreeActiveOnStart =false;
    }
    else if(iCurTabSel == 1) {
        m_bCaseTreeActiveOnStart = true;
    }

    OnSetCapiText(NULL,0);

    // redraw the title
    SetWindowText(pDoc->MakeTitle());

    m_bOnStop = bRet;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnPause()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnPause()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    // ignore, if no application open
    if (pRunApl == NULL) {
        UpdateToolBar();
        return;
    }

    // toggle
    m_bPause = !m_bPause;
    UpdateToolBar();

    APP_MODE appMode = pDoc->GetAppMode();

    if(appMode == ADD_MODE || appMode == VERIFY_MODE || appMode == MODIFY_MODE) { //Temporary Kludge
        CEntryrunView* pView = GetRunView();

        pView->GoToFld((CDEField*)pDoc->GetCurField());
        UpdateToolBar();

        if( pDoc->GetOperatorStatisticsLog() != NULL )
        {
            COperatorStatistics* pObj = pDoc->GetOperatorStatisticsLog()->GetCurrentStatsObj();

            if( pObj != NULL )
            {
                if(m_bPause)
                    pObj->Pause();

                else
                    pObj->Start();
            }
        }
    }

    if( m_keyingInstance != nullptr )
    {
        if( m_bPause )
            m_keyingInstance->Pause();

        else
            m_keyingInstance->UnPause();
    }

    UpdateToolBar();
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateAdd(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateAdd(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
        if(pDoc->GetPifFile()==NULL){
                pCmdUI->Enable(FALSE);
        return;
        }
    if(pDoc && pDoc->GetPifFile() && pDoc->GetPifFile()->GetAddLockFlag()){
        pCmdUI->SetCheck(0);
        pCmdUI->Enable(FALSE);
        return;
    }
    else {
        pCmdUI->Enable(TRUE);
    }
    if(pDoc->GetAppMode() == ADD_MODE)
        pCmdUI->SetCheck(1);
    else
        pCmdUI->SetCheck(0);

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateModify(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateModify(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
        if(pDoc->GetPifFile()==NULL){
                pCmdUI->Enable(FALSE);
        return;
        }
    if(pDoc && pDoc->GetPifFile() && pDoc->GetPifFile()->GetModifyLockFlag()){
        pCmdUI->SetCheck(0);
        pCmdUI->Enable(FALSE);
        return;
    }
    else {
        pCmdUI->Enable(TRUE);
    }
    if(pDoc->GetAppMode() == MODIFY_MODE)
        pCmdUI->SetCheck(1);
    else
        pCmdUI->SetCheck(0);


}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdatePause(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdatePause(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    if(pDoc->GetAppMode() == NO_MODE) {
        pCmdUI->Enable(FALSE);
    }

    pCmdUI->SetCheck(m_bPause);

    if( pDoc->GetAppMode() != NO_MODE && pDoc->GetRunApl()->GetEntryDriver()->HasUserbar() )
        pDoc->GetRunApl()->PauseUserbar(m_bPause);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::UpdateToolBar()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::UpdateToolBar()
{

}

QSFView* CMainFrame::GetQTxtView()
{
    ASSERT(m_wndCapiSplitter.GetPane(0, 0)->IsKindOf(RUNTIME_CLASS(QSFView)));
    return (QSFView*)m_wndCapiSplitter.GetPane(0, 0);
}



/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnPrevCase()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnPrevCase()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();

    CCaseView* pView = GetCaseView();
    if(!pView)
        return;
    CTreeCtrl &treeCtrl = pView->GetTreeCtrl();
    if(treeCtrl.GetCount() && pRunDoc->GetAppMode() == MODIFY_MODE) {
        HTREEITEM hItem = treeCtrl.GetSelectedItem();
        if(hItem) {
            hItem = treeCtrl.GetPrevSiblingItem(hItem);
            if(hItem) {
                treeCtrl.Select(hItem,TVGN_CARET);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdatePrevCase(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdatePrevCase(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnNextCase()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnNextCase()
{
    CCaseView* pView = GetCaseView();
    if(!pView)
        return;

    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CTreeCtrl &treeCtrl = pView->GetTreeCtrl();
    if(treeCtrl.GetCount() && pRunDoc->GetAppMode() == MODIFY_MODE) {
        HTREEITEM hItem = treeCtrl.GetSelectedItem();
        if(hItem) {
            hItem = treeCtrl.GetNextSiblingItem(hItem);
            if(hItem) {
                treeCtrl.Select(hItem,TVGN_CARET);
            }
        }
    }
    return;

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateNextCase(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateNextCase(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnFirstCase()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnFirstCase()
{
    CCaseView* pView = GetCaseView();
    if( !pView )
        return;

    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CTreeCtrl &treeCtrl = pView->GetTreeCtrl();
    if(treeCtrl.GetCount() && pRunDoc->GetAppMode() == MODIFY_MODE) {
        HTREEITEM hItem = treeCtrl.GetRootItem();
        if(hItem) {
            treeCtrl.Select(hItem,TVGN_CARET);
        }
    }
}

void CMainFrame::OnUpdateFirstCase(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

void CMainFrame::OnLastCase()
{
    CCaseView* pView = GetCaseView();
    if( !pView )
        return;

    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();

    CTreeCtrl &treeCtrl = pView->GetTreeCtrl();
    if(treeCtrl.GetCount() && pRunDoc->GetAppMode() == MODIFY_MODE) {
        int iCount = treeCtrl.GetCount();
        HTREEITEM hItem = treeCtrl.GetRootItem();
        for(int iIndex =1; iIndex < iCount ; iIndex ++ ) {
            hItem = treeCtrl.GetNextSiblingItem(hItem);
        }
        treeCtrl.Select(hItem,TVGN_CARET);
    }
}

void CMainFrame::OnUpdateLastCase(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::Start()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::Start()
{
    m_bPause = false;
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnNextGroupOcc()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnNextGroupOcc()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE/* ||pRunDoc->GetAppMode() == VERIFY_MODE*/) {
        if(pRunDoc->GetCurField()) {
            GetRunView()->SendMessage(UWM::CSEntry::SlashKey, 0, (LPARAM)pRunDoc->GetCurField());
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateNextGroupOcc(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateNextGroupOcc(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE /*|| pRunDoc->GetAppMode() == VERIFY_MODE*/) {
        CDEField* pField = (CDEField*)pRunDoc->GetCurField();
        BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
        if(bPathOff) {
            pCmdUI->Enable(bPathOff); //enable only in path off mode
        }
        else {
            pCmdUI->Enable(FALSE);  //path on mode disable EndGroupOcc
            return;
        }
        if(pField && pField->GetDictItem()->GetRecord()->GetSonNumber() == COMMON)
            pCmdUI->Enable(FALSE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnNextGroup()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnNextGroup()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        if(pRunDoc->GetCurField()) {
            GetRunView()->SendMessage(UWM::CSEntry::EndGroup, 0, (LPARAM)pRunDoc->GetCurField());
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateNextGroup(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnUpdateNextGroup(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {

        CDEField* pField = (CDEField*)pRunDoc->GetCurField();
        CDERoster* pRoster  = NULL;
        bool bProcessEndGrp = false;

        BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();

        if(pField) {
            pRoster = DYNAMIC_DOWNCAST(CDERoster,pField->GetParent());
        }
        if(pRoster) {
            bProcessEndGrp = pRoster->UsingFreeMovement();
        }
        if(bProcessEndGrp){
            pCmdUI->Enable(TRUE);
        }
        else if(bPathOff) {
            pCmdUI->Enable(bPathOff); //enable only in path off mode
        }
        else {
            pCmdUI->Enable(FALSE); //disable in path on mode
            return;
        }

        if(pField && pField->GetDictItem()->GetRecord()->GetSonNumber() == COMMON)
            pCmdUI->Enable(FALSE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnNextLevel()
//
/////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnNextLevel()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        if(pRunDoc->GetCurField()) {
            if(pRunDoc->GetRunApl()->GetCurrentLevel() != 1 ){
                GetRunView()->SendMessage(UWM::CSEntry::EndLevel, 0, (LPARAM)pRunDoc->GetCurField());
            }
            else if (pRunDoc->GetRunApl()->GetCurrentLevel() == 1 ){
                OnNextLevelOcc();
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateNextLevel(CCmdUI* pCmdUI)
//
// this func is called to determine if ctrl+F12 should be active; this backs you
// out of the current level; if in a one-level app, or in level 1 of a multi-level
// app, this will finish the case and advance to the next case
//
// op-controlled: -can always be called while in modify mode
//                                -can not be called while in verify mode
//                -in add mode, can only be called if NONE, or ALL, of the
//                                 current level IDs have been keyed; inactive if IDs have
//                 been partially keyed;
// sys-controlled: ask macro; i'm allowing in modify mode, disallowing in verify,
//                 and more-or-less following what was here before for add mode
//
/////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnUpdateNextLevel(CCmdUI* pCmdUI)
{
        CEntryrunDoc* pRunDoc = (CEntryrunDoc*) GetActiveDocument();
        if (pRunDoc->GetAppMode() == MODIFY_MODE){
                pCmdUI->Enable (TRUE);
        }
        else if (pRunDoc->GetAppMode() == ADD_MODE) {

                BOOL bPathOn = pRunDoc->GetCurFormFile()->IsPathOn();

                if (bPathOn)    // then can't do it

                        pCmdUI->Enable(FALSE);

                else {
                        CRunAplEntry*   pRunApl = pRunDoc->GetRunApl();
                        CDEField*       pField = (CDEField*) pRunDoc->GetCurField();

                        if (pRunApl != NULL && pField != NULL ) {
                                int iCurLevel = pRunApl->GetCurrentLevel();
                                if(iCurLevel >1) {//SAVY  GSF Request 07/28/04
                                        pCmdUI->Enable(TRUE);
                                }
                                else if (pRunApl->IsNewCase()){       // smg
                                        pCmdUI->Enable( FALSE );
                                }//SAVY  GSF Request 07/28/04
                                /*else if (pRunApl->QidReady (iCurLevel)){ //smg
                                        pCmdUI->Enable( TRUE );
                                }*/
                                else{
                                        pCmdUI->Enable( TRUE );
                                }
                        }
                }

        }
        else {      // no mode or verify mode
                pCmdUI->Enable(FALSE);
        }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnNextLevelOcc()
//
/////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnNextLevelOcc()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();

    if (pRunDoc->GetAppMode() == MODIFY_MODE || pRunDoc->GetAppMode() == ADD_MODE)
    {
        if(pRunDoc->GetCurField())
            GetRunView()->SendMessage(UWM::CSEntry::NextLevelOccurrence, 0, (LPARAM)pRunDoc->GetCurField());
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateNextLevelOcc(CCmdUI* pCmdUI)
//
// this func is called to determine if F12 should be active; it allows you to
// advance to the next *node* of the current level (i.e., next occurrence of the level)
//
// op-controlled: -always active in modify mode
//                -always inactive in verify mode
//                -in add mode can only be called if all IDs for the level have been keyed
//                 for modify, is always active
// sys-controlled:  ? for now, leaving what was there before, macro needs to decide this
//
/////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnUpdateNextLevelOcc(CCmdUI* pCmdUI)
{
        CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();

        if ( pRunDoc->GetAppMode() == MODIFY_MODE ){
                pCmdUI->Enable(TRUE);   // F12 is always active in modify mode
        }
        else if (pRunDoc->GetAppMode() == ADD_MODE) {
                BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
                CDEField* pField = (CDEField*) pRunDoc->GetCurField();
                CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

                if (bPathOff) { // smg: only enabled if user has keyed all ID fields

                        if (pRunApl != NULL && pField != NULL ) {
                                pCmdUI->Enable(TRUE);
                                int iCurLevel = pRunApl->GetCurrentLevel();
                                if(iCurLevel >1) {//SAVY  GSF Request 07/28/04
                                        pCmdUI->Enable(TRUE);
                                }
                                else if (pRunApl->IsNewCase()){// smg: this makes f12 work when it's supposed to!
                                        pCmdUI->Enable( FALSE );
                                }//SAVY  GSF Request 09/01/04
                                /*else if (pRunApl->QidReady (iCurLevel)){ //smg
                                        pCmdUI->Enable( TRUE );
                                }*/
                                else{
                                        pCmdUI->Enable( TRUE );
                                }

                        }
                }
                else { //path on  smg: i've left what was written, dunno if correct

                        if (pField && !pRunApl->IsNewCase() ) {
                                if (pRunApl->GetCurrentLevel() == 1 &&
                                        pRunDoc->GetCurFormFile()->GetNumLevels() > 1) {

                                                if(pRunDoc->GetAppMode() == ADD_MODE) {

                                                        pCmdUI->Enable(TRUE);
                                                }
                                                else {
                                                        pCmdUI->Enable(FALSE);
                                                }
                                        }
                                else {
                                        pCmdUI->Enable(TRUE);
                                }
                        }
                        else {
                                pCmdUI->Enable(FALSE);
                        }
                }
        }
        else
                pCmdUI->Enable(FALSE);  // can't use F12 unless in modify or add mode
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnPrevScreen()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnPrevScreen()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        if(pRunDoc->GetCurField()) {
            GetRunView()->SendMessage(UWM::CSEntry::PreviousPage, 0, (LPARAM)pRunDoc->GetCurField());
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdatePrevScreen(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdatePrevScreen(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
        if(bPathOff) {
            pCmdUI->Enable(bPathOff); //enable only in path off mode

        }
        else {
            pCmdUI->Enable(FALSE); //disbale in path on mode
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnNextScreen()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnNextScreen()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        if(pRunDoc->GetCurField()) {
            GetRunView()->SendMessage(UWM::CSEntry::NextPage, 0, (LPARAM)pRunDoc->GetCurField());
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateNextScreen(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateNextScreen(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
        if(bPathOff) {
            pCmdUI->Enable(bPathOff); //enable only in path off mode

        }
        else {
            pCmdUI->Enable(FALSE); //disbale in path on mode
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnDeletecase()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnDeletecase()
{
    GetCaseView()->OnDeleteCase();
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateDeletecase(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateDeletecase(CCmdUI* pCmdUI)
{
    BOOL bEnable = FALSE;

    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CNPifFile* pPifFile = pRunDoc->GetPifFile();

    if( pPifFile != nullptr && !pPifFile->GetDeleteLockFlag() )
    {
        if( GetCaseView()->GetTreeCtrl().GetCount() > 0 )
        {
            HTREEITEM hItem = GetCaseView()->GetTreeCtrl().GetSelectedItem();

            if( pRunDoc->GetAppMode() == NO_MODE && hItem != nullptr )
                bEnable = TRUE;
        }
    }

    pCmdUI->Enable(bEnable);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CMainFrame::OnWriteCase(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::OnWriteCase(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    const Case& data_case = *(const Case*)lParam;

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    COperatorStatisticsLog* pOperatorStatisticsLog = pDoc->GetOperatorStatisticsLog();

    if( pOperatorStatisticsLog != nullptr && pOperatorStatisticsLog->GetCurrentStatsObj() != nullptr )
    {
        if( pDoc->GetAppMode() == ADD_MODE || pDoc->GetAppMode() == VERIFY_MODE )
        {
            //Get the opstats object
            int iNumRecs = data_case.GetPre74_Case()->CalculateRecordsForWriting();
            int iNumCases = pOperatorStatisticsLog->GetCurrentStatsObj()->GetTotalCases();

            iNumCases++;
            pOperatorStatisticsLog->GetCurrentStatsObj()->SetTotalCases(iNumCases);

            int iTotalRecs = pOperatorStatisticsLog->GetCurrentStatsObj()->GetTotalRecords();
            iTotalRecs += iNumRecs;
            pOperatorStatisticsLog->GetCurrentStatsObj()->SetTotalRecords(iTotalRecs);

            pOperatorStatisticsLog->Save();
        }
    }

    if( m_keyingInstance != nullptr )
        m_keyingInstance->SetRecordsWritten(data_case.GetPre74_Case()->CalculateRecordsForWriting());


    // update the case listing
    BuildKeyArray();

    if( pDoc->GetAppMode() == ADD_MODE )
        GetCaseView()->BuildTree();

    return 0L;
}


LONG CMainFrame::OnKeyChanged(WPARAM wParam, LPARAM /*lParam*/)
{
    // sent by the engine after a partial save (which changed the case key) or from setcaselabel
    const Case& data_case = *reinterpret_cast<const Case*>(wParam);

    CCaseView* pCaseView = GetCaseView();
    CTreeCtrl& caseTree = pCaseView->GetTreeCtrl();
    HTREEITEM hSelItem = caseTree.GetSelectedItem();

    CString csNewKey = GetShowCaseLabels() ? data_case.GetCaseLabelOrKey() : data_case.GetKey();
    NewlineSubstitutor::MakeNewlineToUnicodeNL(csNewKey);
    caseTree.SetItemText(hSelItem, csNewKey);

    if( data_case.IsPartial() )
    {
        int iImageIndex = pCaseView->PartialSaveStatusModeToImageIndex(data_case.GetPartialSaveMode());
        caseTree.SetItemImage(hSelItem, iImageIndex, iImageIndex);
    }

    return 0;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CMainFrame::OnPreprocessEngineMessage(WPARAM wParam, LPARAM lParam)
//      LONG CMainFrame::OnEngineMessage(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////

LONG CMainFrame::OnPreprocessEngineMessage(WPARAM wParam, LPARAM lParam)
{
    auto message_type = (MessageType)wParam;
    auto message_number = (int)lParam;

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    // Add out of range errors to entry error count
    eMsgType display_msg_type = eMsgType::ISSA;

    if( message_type == MessageType::Error )
    {
        switch( message_number )
        {
            case MGF::OutOfRangeConfirm:
            case MGF::OutOfRange:
                display_msg_type = eMsgType::OutOfRange;
                pDoc->AddEntryError();
                break;
        }
    }

    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    if( pRunApl == nullptr )
    {
        ASSERT(false);
        return -1;
    }

    // special processing if in interactive edit
    if( pDoc->GetIntEdit() )
    {
        // out of range errors are processed in CsDriver::AcceptFieldValue
        if( display_msg_type != eMsgType::OutOfRange )
        {
            // if stopping on error messages, stop the advance
            if( intEdtDlg.m_iOption == 2 || intEdtDlg.m_iOption == 0 )
                pRunApl->SetStopAdvance(true);

            // otherwise, quit without showing the message
            if( intEdtDlg.m_iOption == 1 )
                return -1;
        }
    }

    return 1;
}


LONG CMainFrame::OnEngineMessage(WPARAM wParam, LPARAM/* lParam*/)
{
    const CMsgOptions& message_options = *reinterpret_cast<const CMsgOptions*>(wParam);
    CEntryrunDoc* pDoc = assert_cast<CEntryrunDoc*>(GetActiveDocument());

    // Add out of range errors to entry error count
    eMsgType display_msg_type = eMsgType::ISSA;

    if( message_options.GetMessageType().value_or(MessageType::User) == MessageType::Error && message_options.GetMessageNumber().has_value() )
    {
        switch( *message_options.GetMessageNumber() )
        {
            case MGF::OutOfRangeConfirm:
            case MGF::OutOfRange:
                display_msg_type = eMsgType::OutOfRange;
                pDoc->AddEntryError();
                break;
        }
    }

    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    if( pRunApl == nullptr )
    {
        ASSERT(false);
        return 0;
    }

    // there are two error messages styles but they can be overriden
    const MessageOverrides& message_overrides = pDoc->GetMessageOverrides();

    bool use_operator_controlled_style = message_overrides.ForceOperatorControlled() ||
                                         ( !pRunApl->IsPathOn() && !message_overrides.ForceSystemControlled() );

    // buttons don't work with the operator controlled style
    const bool using_buttons = ( display_msg_type == eMsgType::ISSA && !message_options.GetArrayButtons().empty() );

    if( use_operator_controlled_style && using_buttons )
        use_operator_controlled_style = false;

    // special processing if in interactive edit
    if( pDoc->GetIntEdit() )
    {
        // out of range errors are processed in CsDriver::AcceptFieldValue
        if( display_msg_type != eMsgType::OutOfRange )
        {
            // if stopping on error messages, stop the advance
            if( intEdtDlg.m_iOption == 2 || intEdtDlg.m_iOption == 0 )
                pRunApl->SetStopAdvance(true);

            // otherwise, quit without showing the message
            if( intEdtDlg.m_iOption == 1 && !using_buttons )
                return 0;
        }
    }

    // 20110802 trevor pointed out that the screen isn't refreshed before messages, so that if
    // the logic changes any of the fields on the form, they won't appear until after all the logic runs
    OnEngineRefresh(0, 0);

    // display the message
    ::MessageBeep(0);

    if( use_operator_controlled_style )
    {
        CCustMsg operator_controlled_message_dlg(pDoc->GetMessageOverrides(), &message_options);

        operator_controlled_message_dlg.m_eMsgType = display_msg_type;
        operator_controlled_message_dlg.m_sMessage = message_options.GetMsg();
        operator_controlled_message_dlg.m_bCanForceOutOfRange = ( display_msg_type == eMsgType::OutOfRange &&
                                                                  *message_options.GetMessageNumber() == MGF::OutOfRangeConfirm );

        operator_controlled_message_dlg.ShowMessage();

        return operator_controlled_message_dlg.GetRetVal();
    }

    // system controlled style
    else
    {
        CMsgDialog system_controlled_message_dlg(message_options);

        system_controlled_message_dlg.SetTextLabelFontHeigth(16);
        system_controlled_message_dlg.SetTextLabelFontWidth(8);

        system_controlled_message_dlg.DoModal();

        return system_controlled_message_dlg.GetLastPressedButtonIndex();
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnInsertGroupocc()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnInsertGroupocc()
{
    GetRunView()->SendMessage(WM_COMMAND,ID_INSERT_GROUPOCC);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateInsertGroupocc(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateInsertGroupocc(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        CDEField* pCurField = (CDEField*)pRunDoc->GetCurField();
        if(pCurField) {
            int iMaxOccs = pCurField->GetParent()->GetMaxLoopOccs();
            int iCompare = pCurField->GetParent()->GetTotalOccs();
            if( pRunDoc->IsAutoEndGroup() ) { // RHF Nov 07, 2000
                if(mode == MODIFY_MODE ) {
                    iCompare = pCurField->GetParent()->GetDataOccs();
                }
            }
            if(iMaxOccs > 1  && iCompare <= iMaxOccs){
                pCmdUI->Enable(TRUE);
            }
        }
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnDeleteGrpocc()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnDeleteGrpocc()
{
    GetRunView()->SendMessage(WM_COMMAND,ID_DELETE_GRPOCC);

}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateDeleteGrpocc(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateDeleteGrpocc(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        CDEField* pCurField = (CDEField*)pRunDoc->GetCurField();
        if(pCurField) {
            int iMaxOccs = pCurField->GetParent()->GetMaxLoopOccs();
            int iCompare = pCurField->GetParent()->GetTotalOccs();
            if( pRunDoc->IsAutoEndGroup() ) { // RHF Nov 07, 2000
                if(mode == MODIFY_MODE ) {
                    iCompare = pCurField->GetParent()->GetDataOccs();
                }
            }
            if(iMaxOccs > 1  && iCompare <= iMaxOccs){
                pCmdUI->Enable(TRUE);
            }
        }
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnSortgrpocc()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnSortgrpocc()
{
    GetRunView()->SendMessage(WM_COMMAND,ID_SORTGRPOCC);

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateSortgrpocc(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateSortgrpocc(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        CDEField* pCurField = (CDEField*)pRunDoc->GetCurField();
        if(pCurField) {
            int iMaxOccs = pCurField->GetParent()->GetMaxLoopOccs();
            int iCompare = pCurField->GetParent()->GetTotalOccs();
            if( pRunDoc->IsAutoEndGroup() ) { // RHF Nov 07, 2000
                if(mode == MODIFY_MODE ) {
                    iCompare = pCurField->GetParent()->GetDataOccs();
                }
            }
            if(iMaxOccs > 1  && iCompare <= iMaxOccs){
                pCmdUI->Enable(TRUE);
            }
        }
    }

}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateInsertCase(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateInsertCase(CCmdUI* pCmdUI)
{
    BOOL bEnable = FALSE;
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CNPifFile* pPifFile = pRunDoc->GetPifFile();

    if( pPifFile != nullptr && !pPifFile->GetAddLockFlag() )
    {
        if( GetCaseView()->GetTreeCtrl().GetCount() > 0 )
        {
            HTREEITEM hItem = GetCaseView()->GetTreeCtrl().GetSelectedItem();

            if( pRunDoc->GetAppMode() == NO_MODE && hItem != nullptr )
                bEnable = TRUE;
        }
    }

    pCmdUI->Enable(bEnable);
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnInsertAfterOcc()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnInsertAfterOcc()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        if(pRunDoc->GetCurField()) {
            GetRunView()->SendMessage(UWM::CSEntry::InsertAfterOccurrence, 0, (LPARAM)pRunDoc->GetCurField());
        }
    }

}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateInsertGrpoccAfter(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateInsertGrpoccAfter(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*    pRunApl = pRunDoc->GetRunApl();
    pCmdUI->Enable(FALSE);
    if(!pRunApl) {
        return;
    }

    APP_MODE mode =  pRunDoc->GetAppMode();
    if(mode != ADD_MODE && mode != MODIFY_MODE) {
        return;
    }
    else {
        CDEField* pCurField = (CDEField*)pRunDoc->GetCurField();
        if(pCurField) {
            int iMaxOccs = pCurField->GetParent()->GetMaxLoopOccs();
            int iDataOccs = pCurField->GetParent()->GetDataOccs();
            if(iMaxOccs > 1  && iDataOccs < iMaxOccs){
                pCmdUI->Enable(TRUE);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnFindcase()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnFindcase()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    if( pRunApl == NULL || pRunDoc->GetAppMode() != NO_MODE )
        return;

    CTreeCtrl& caseTree = GetCaseView()->GetTreeCtrl();

    HTREEITEM hItem = caseTree.GetSelectedItem();

    if( hItem == NULL )
    {
        hItem = caseTree.GetRootItem();

        if( hItem == NULL )
            return;
    }

    NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItem);

    CQuestionnaireSearchDlg questionnaireSearchDlg(pNodeInfo->case_summary.GetKey());

    if( questionnaireSearchDlg.DoModal() == IDOK )
    {
        if( !pRunDoc->GetPifFile()->GetModifyLockFlag() )
            OnModifyCase();
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateFindcase(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateFindcase(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
    BOOL bEnable = FALSE;

    if( pRunApl != NULL && pRunDoc->GetAppMode() == NO_MODE )
    {
        int iCount = GetCaseView()->GetTreeCtrl().GetCount();

        if( iCount > 0 )
            bEnable = TRUE;
    }

    pCmdUI->Enable(bEnable);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnPreviousPersistent()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnPreviousPersistent()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        if(pRunDoc->GetCurField()) {
            GetRunView()->SendMessage(UWM::CSEntry::PreviousPersistent);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdatePreviousPersistent(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdatePreviousPersistent(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        BOOL bPathOff = !pRunDoc->GetCurFormFile()->IsPathOn();
        if(bPathOff) {
            pCmdUI->Enable(bPathOff); //enable only in path off mode
        }
        else {
            pCmdUI->Enable(FALSE);  //path on mode disable EndGroupOcc
            return;
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateVerify(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateVerify(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pDoc->GetPifFile() == NULL){
        pCmdUI->Enable(FALSE);
        return;
    }

    BOOL bVerifyFlag = pDoc->GetPifFile()->GetVerifyLockFlag();
    if(pDoc && pDoc->GetPifFile() && bVerifyFlag){
        pCmdUI->SetCheck(0);
        pCmdUI->Enable(FALSE);
        return;
    }
    else {
        pCmdUI->Enable(TRUE);
    }
    if(pDoc->GetAppMode() == VERIFY_MODE)
        pCmdUI->SetCheck(1);
    else
        pCmdUI->SetCheck(0);

}

/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CMainFrame::OnEngineAbort(WPARAM wParam, LPARAM lParam)//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::OnEngineAbort(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // AfxMessageBox(_T("Implement clean up"));
    // 20140514 tom hounded us enough to change the above, legendary message, so here is a new one:
    AfxMessageBox(_T("A fatal error occurred from which CSEntry could not recover. If this error occurs repeatedly, please email: cspro@lists.census.gov"));

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    pDoc->SetQModified(FALSE); // suppress any warnings about saving data when exiting

    AfxGetMainWnd()->SendMessage(WM_CLOSE);
    return 0L;
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnGoto()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnGoto()
{
    CGoToDlg dlg;
    if(dlg.DoModal() == IDOK){
        ASSERT(dlg.m_pBase != NULL);
        CDEField* pField = DYNAMIC_DOWNCAST(CDEField,dlg.m_pBase);
        ASSERT(pField != NULL);
        CEntryrunView* pView = GetRunView();
        CWaitCursor wait;
        pView->GoToField(pField,dlg.m_iOcc);
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateGoto(CCmdUI* pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateGoto(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pDoc->GetRunApl() && pDoc->GetAppMode() != NO_MODE && pDoc->GetAppMode() != VERIFY_MODE){
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                             CMainFrame::OnMenuChar
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
    LRESULT lresult;
    if(BCMenu::IsMenu(pMenu)) {
        lresult=BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
    }
    else {
        lresult=CFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
    }
    return(lresult);
}

void CMainFrame::OnAdvtoend()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        if(pRunDoc->GetCurField()) {
            GetRunView()->SendMessage(UWM::CSEntry::AdvanceToEnd, 0, (LPARAM)pRunDoc->GetCurField());// RHF Jan 25, 2001
            return;
        }
    }
}

void CMainFrame::OnUpdateAdvtoend(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    if(pRunDoc->GetAppMode() == MODIFY_MODE ||pRunDoc->GetAppMode() == ADD_MODE) {
        CDEField* pField = (CDEField*)pRunDoc->GetCurField();
        if(pField && pRunDoc->GetRunApl()->IsPathOn()) {
            pCmdUI->Enable(TRUE);
        }
        else {
            pCmdUI->Enable(FALSE);
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}

void CMainFrame::OnInsertNode()
{
    GetCaseView()->InsertNode();
}

void CMainFrame::OnAddNode()
{
    GetCaseView()->AddNode();
}

void CMainFrame::OnUpdateAddInsertNode(CCmdUI* pCmdUI)
{
    CTreeCtrl& caseTree = GetCaseView()->GetTreeCtrl();
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CNPifFile* pPifFile = pRunDoc->GetPifFile();
    BOOL bEnable = FALSE;

    if( caseTree.GetCount() > 0 && pRunDoc->GetAppMode() == NO_MODE && pPifFile != nullptr )
    {
        HTREEITEM hItem = caseTree.GetSelectedItem();

        if( hItem != nullptr && !pPifFile->GetModifyLockFlag() )
        {
            NODEINFO* pNodeInfo  = (NODEINFO*)caseTree.GetItemData(hItem);
            bEnable = ( pNodeInfo != nullptr && pNodeInfo->iNode > 0 ); // only able to add/insert level 2+ nodes
        }
    }

    pCmdUI->Enable(bEnable);
}


void CMainFrame::OnDeleteNode()
{
    GetCaseView()->DeleteNode();
}

void CMainFrame::OnUpdateDeleteNode(CCmdUI* pCmdUI)
{
    CTreeCtrl& caseTree = GetCaseView()->GetTreeCtrl();
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CNPifFile* pPifFile = pRunDoc->GetPifFile();
    BOOL bEnable = FALSE;

    if( caseTree.GetCount() > 0 && pRunDoc->GetAppMode() == NO_MODE && pPifFile != nullptr )
    {
        HTREEITEM hItem = caseTree.GetSelectedItem();

        if( hItem != nullptr && !pPifFile->GetDeleteLockFlag() )
        {
            NODEINFO* pNodeInfo  = (NODEINFO*)caseTree.GetItemData(hItem);
            bEnable = ( pNodeInfo != nullptr && pNodeInfo->iNode > 0 ); // only able to delete level 2+ nodes
        }
    }

    pCmdUI->Enable(bEnable);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdatePartialsInd(CCmdUI *pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdatePartialsInd(CCmdUI *pCmdUI)
{
    CString csNumPartials;

    if( m_iTotalPartial > 0 )
        csNumPartials.Format( _T("%d Partials"),m_iTotalPartial);

    else
        csNumPartials.Format( _T("No Partials"));

    pCmdUI->SetText(csNumPartials);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateFieldInd(CCmdUI *pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateFieldInd(CCmdUI *pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(!pDoc) {
        pCmdUI->Enable(false);
    }
    else {
        CDEItemBase* pField = pDoc->GetCurField();

        if( pField == NULL || pDoc->GetAppMode() == NO_MODE){
            pCmdUI->Enable(false);
            return;
        }

        pCmdUI->Enable(true);
        CString sOcc;


        sOcc.Format( _T("Field = %s"), (LPCTSTR)pField->GetName());

        // 20120305 to show the number of characters entered for the new unicode/multiline controls
        CDEField * pActualField = (CDEField *)pField;

        if( pActualField && pActualField->UseUnicodeTextBox() )
        {
            CDEBaseEdit * pEdit = GetRunView()->SearchEdit(pActualField);
            CString windowText;

            pEdit->CWnd::GetWindowText(windowText);

            sOcc.AppendFormat(_T(", Length: %d/%d"),windowText.GetLength(),pActualField->GetDictItem()->GetLen());
        }

        pCmdUI->SetText(sOcc);
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateOccInd(CCmdUI *pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateOccInd(CCmdUI *pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(!pDoc) {
        pCmdUI->Enable(false);
    }
    else {
        if(pDoc->GetCurField() == NULL || pDoc->GetAppMode() == NO_MODE){
            pCmdUI->Enable(false);
            return;
        }
        int iCurrOcc = pDoc->GetCurField()->GetParent()->GetCurOccurrence();
        int iTotalOccs = pDoc->GetCurField()->GetParent()->GetDataOccs();
        if(iTotalOccs < iCurrOcc){
            iTotalOccs = iCurrOcc;
        }
        pCmdUI->Enable(true);
        CString sOcc;
        sOcc.Format( _T("Occurrence %d of %d"), iCurrOcc,iTotalOccs);
        pCmdUI->SetText(sOcc);
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateModeInd(CCmdUI *pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateModeInd(CCmdUI *pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(!pDoc) {
        pCmdUI->Enable(false);
    }
    else {
        CString sMode;
        if(pDoc->GetAppMode() == NO_MODE){
            pCmdUI->Enable(false);
            return;
        }
        else if(pDoc->GetAppMode() == ADD_MODE){
            sMode= _T("ADD");
        }
        else if(pDoc->GetAppMode() == MODIFY_MODE){
            sMode = _T("MODIFY");
        }
        else if(pDoc->GetAppMode() == VERIFY_MODE){
            sMode = _T("VERIFY");
        }
        pCmdUI->Enable(true);
        pCmdUI->SetText(sMode);
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateShowInd(CCmdUI *pCmdUI)
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateShowInd(CCmdUI *pCmdUI)        // BMD 05 Mar 2002
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if(!pDoc) {
        pCmdUI->Enable(false);
    }
    else {
        CString sMode = _T("");
        if(pDoc->GetAppMode() != VERIFY_MODE){
            pCmdUI->Enable(false);
            return;
        }
        CEntryrunView* pView = GetRunView();

        if (pView->GetCheatKey()) {
            sMode = _T("Show");
        }
        pCmdUI->Enable(true);
        pCmdUI->SetText(sMode);
    }
}


void CMainFrame::OnFullscreen()
{
    // 20140405 added a third option, so changing from m_bFullScreen to m_eScreenView
    LockWindowUpdate();

    if( m_eScreenView == FULLSCREEN_NO ) // from the default view (with a case/file tree) to the original full screen mode
    {
        ASSERT( m_pLeftView!=NULL );
        CRect rect;
        ::GetWindowRect(m_pLeftView->GetSafeHwnd(),&rect);
        m_iWidth = rect.Width();
        m_wndSplitter.SetColumnInfo(0,0,0);
        m_wndSplitter.GetPane(0, 0)->ShowWindow(SW_HIDE);
        m_wndSplitter.SetActivePane(0, 1);
        m_eScreenView = FULLSCREEN_YES;
    }

    else if( m_eScreenView == FULLSCREEN_YES ) // from the original full screen to a tablet mode (with no menus)
    {
        SetMenu(NULL);
        ModifyStyle(WS_CAPTION,0); // to hide
        m_wndReBar.ShowWindow(SW_HIDE);
        m_eScreenView = FULLSCREEN_NOMENUS;
    }

    else // from tablet mode to the default view
    {
        m_wndSplitter.SetColumnInfo(0,m_iWidth,0);
        m_wndSplitter.GetPane(0, 0)->ShowWindow(SW_SHOW);
        ModifyStyle(0,WS_CAPTION); // to show
        m_wndReBar.ShowWindow(SW_SHOW);
        SetMenu(&m_menu);
        m_eScreenView = FULLSCREEN_NO;
    }

    m_wndSplitter.RecalcLayout();

    UnlockWindowUpdate();

    CDC* pDC = GetWindowDC();
    SetWindowPos( NULL, 0, 0, GetDeviceCaps(pDC->GetSafeHdc(), HORZRES), GetDeviceCaps(pDC->GetSafeHdc(), VERTRES), SWP_FRAMECHANGED);
}

void CMainFrame::OnUpdateFullscreen(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
    pCmdUI->SetCheck(m_eScreenView != FULLSCREEN_NO);
}

void CMainFrame::OnShowCaseLabels()
{
    m_bShowCaseLabels = !m_bShowCaseLabels;
    GetCaseView()->BuildTree();
}

void CMainFrame::OnUpdateShowCaseLabels(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    BOOL bEnable = ( pDoc != NULL && pDoc->GetAppMode() == NO_MODE );

    pCmdUI->Enable(bEnable);
    pCmdUI->SetCheck(m_bShowCaseLabels);
}

// ***************************************************************
// void CMainFrame::OnToggleNames(void)
//
// this func added by smg on 21 may 03 to support toggling of the
// names in the case tree between labels and uniq names
// ***************************************************************

void CMainFrame::OnToggleNames(void)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    bool          bTreeEnabled = pDoc && (  pDoc->GetAppMode()==MODIFY_MODE ||
                                            pDoc->GetAppMode()==ADD_MODE ||
                                            pDoc->GetAppMode()==VERIFY_MODE );
    CCaseTree*    pCaseTree = GetCaseTree();

    if (bTreeEnabled && pCaseTree) {
            pCaseTree->ChangeView();
                m_bViewNames = !m_bViewNames;   // toggle the view
    }
}

void CMainFrame::OnUpdateToggleNames(CCmdUI* pCmdUI)
{
    CCaseTree*    pCaseTree = GetCaseTree();
    int iIndex = -1;
    CLeftPropSheet* pPropSheet  = m_pLeftView   ? m_pLeftView->GetPropSheet()   : NULL;
    if(pPropSheet) {
        iIndex = pPropSheet->GetActiveIndex();
    }
    if(iIndex !=1 ) {
       pCmdUI->SetCheck (m_bViewNames);
       pCmdUI->Enable (FALSE);
       return;
    }
    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetActiveDocument();
    bool          bTreeEnabled = pDoc && ( pDoc->GetAppMode()==MODIFY_MODE ||
                                                                                        pDoc->GetAppMode()==ADD_MODE ||
                                                                                        pDoc->GetAppMode()==VERIFY_MODE );

    pCmdUI->SetCheck(m_bViewNames);
    pCmdUI->Enable(bTreeEnabled && pCaseTree);
}

// ***************************************************************

void CMainFrame::OnSortorder()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    if( pDoc != NULL && pDoc->GetRunApl() && !pDoc->GetRunApl()->HasStarted() )
    {
        SetSortFlag(!GetSortFlag());

        CCaseView* pCaseView = GetCaseView();

        HTREEITEM hItem = ( pCaseView != NULL ) ? pCaseView->GetTreeCtrl().GetSelectedItem() : NULL;

        CString csKey;

        if( hItem != NULL )
        {
            NODEINFO* pNodeInfo = (NODEINFO*)pCaseView->GetTreeCtrl().GetItemData(hItem);
            csKey = pNodeInfo->case_summary.GetKey();
        }

        BuildKeyArray();

        if( pCaseView != NULL )
            pCaseView->BuildTree();

        if( !csKey.IsEmpty() )
            pCaseView->RestoreSelectPos(csKey);
    }
}

void CMainFrame::OnUpdateSortorder(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    if( pDoc != NULL && pDoc->GetRunApl() && !pDoc->GetRunApl()->HasStarted() )
    {
        pCmdUI->Enable(TRUE);
        pCmdUI->SetCheck(GetSortFlag() ? TRUE : FALSE);
    }

    else
        pCmdUI->Enable(FALSE);
}


bool CaseIsValidForVerification(const CaseSummary& case_summary)
{
    return
        !case_summary.GetVerified() &&
        (   case_summary.GetPartialSaveMode() == PartialSaveMode::None ||
            case_summary.GetPartialSaveMode() == PartialSaveMode::Verify
        );
}

bool CMainFrame::SelectNextCaseForVerification()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    Application* pApp = pDoc->GetPifFile()->GetApplication();

    int iFrequency = pApp->GetVerifyFreq();
    int iStartCase = pApp->GetVerifyStart();

    CTreeCtrl& caseTree = GetCaseView()->GetTreeCtrl();
    HTREEITEM hItemToVerify = NULL;

    if( iFrequency == 1 )
    {
        // if the frequency is 1, first see if the currently selected case is not verified; if so, then verify it;
        // if not, then find the first non-verified case; the start case should always be 1 when the frequency is 1
        ASSERT(iStartCase == 1);

        HTREEITEM hItemSelected = caseTree.GetSelectedItem();
        HTREEITEM hItemIterator = caseTree.GetRootItem();
        bool bReachedSelectedItem = ( hItemIterator == hItemSelected );

        while( hItemIterator != NULL )
        {
            NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItemIterator);

            bReachedSelectedItem |= ( hItemIterator == hItemSelected );

            if( CaseIsValidForVerification(pNodeInfo->case_summary) )
            {
                if( hItemIterator == hItemSelected )
                    return true; // we can return since the selected item is valid for verification

                if( hItemToVerify == NULL )
                    hItemToVerify = hItemIterator;
            }

            if( bReachedSelectedItem && ( hItemToVerify != NULL ) )
                break;

            hItemIterator = caseTree.GetNextSiblingItem(hItemIterator);
        }
    }

    else
    {
        // if the frequency is greater than 1, we will first find the verified high water mark and then use the frequency
        // number to figure out what case to verify; this could lead to odd behavior if cases get inserted at some point

        // if the user wants a random start, calculate that start
        if( iStartCase == -1 )
        {
            if( iFrequency < 1 || iFrequency > 99 )
            {
                iFrequency = 1;
                iStartCase = 1;
            }

            else
            {
                std::srand((unsigned int)std::time(0));
                int iMax = std::min(iFrequency, (int)m_caseSummaries.size());
                iStartCase = ( std::rand() % iMax ) + 1;
            }
        }

        if( iStartCase > (int)m_caseSummaries.size() )
        {
            CString csMsg;
            csMsg.Format(_T("The Start Case index is greater than the number of cases (%d > %d). Cannot start verify mode."), iStartCase, (int)m_caseSummaries.size());
            AfxMessageBox(csMsg);
            return false;
        }

        // find the verification high water mark
        HTREEITEM hItemLastVerified = NULL;
        HTREEITEM hItemIterator = caseTree.GetRootItem();

        while( hItemIterator != NULL )
        {
            NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItemIterator);

            if( pNodeInfo->case_summary.GetVerified() )
                hItemLastVerified = hItemIterator;

            hItemIterator = caseTree.GetNextSiblingItem(hItemIterator);
        }

        // reposition the iterator at the root or at the first eligible case after the last verified case
        int iCasesCountdown = 0;

        if( hItemLastVerified == NULL )
        {
            hItemIterator = caseTree.GetRootItem();
            iCasesCountdown = iStartCase;
        }

        else
        {
            hItemIterator = caseTree.GetNextSiblingItem(hItemLastVerified);
            iCasesCountdown = iFrequency;
        }

        while( hItemIterator != NULL )
        {
            NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hItemIterator);

            iCasesCountdown--;

            if( CaseIsValidForVerification(pNodeInfo->case_summary) && iCasesCountdown <= 0 )
            {
                hItemToVerify = hItemIterator;
                break;
            }

            hItemIterator = caseTree.GetNextSiblingItem(hItemIterator);
        }
    }

    if( hItemToVerify == NULL )
    {
        AfxMessageBox(MGF::GetMessageText(MGF::VerifyDone).c_str());
        return false;
    }

    else
    {
        caseTree.Select(hItemToVerify,TVGN_CARET);
        return true;
    }
}


// SERPRO
// RHF INIC Nov 19, 2001
LONG CMainFrame::OnEngineRefresh(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CEntryrunView*  pView=GetRunView();
    CEntryrunDoc*   pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*   pRunApl = NULL;

    if( pView == NULL || pDoc == NULL || (pRunApl = pDoc->GetRunApl()) == NULL )
        return 0;

    pView->UpdateFields();
    pView->RedrawWindow();

    return 0;
}
// RHF END Nov 19, 2001

LONG CMainFrame::OnEngineShowCapi(WPARAM wParam, LPARAM lParam) {

    bool bRefreshForLanguageChange = lParam != 0;

    // 20100622 keep this function from running over and over
    MSG msg;
    if( PeekMessage(&msg,this->GetSafeHwnd(), UWM::CSEntry::ShowCapi, UWM::CSEntry::ShowCapi, PM_NOREMOVE) )
        return 0;

    CEntryrunView*  pView=GetRunView();
    CEntryrunDoc*   pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*   pRunApl = NULL;

    if( pView == NULL || pDoc == NULL || (pRunApl = pDoc->GetRunApl()) == NULL  )
        return 0;

    CDEField*   pField=(CDEField*)pDoc->GetCurField();
    CDEBaseEdit*    pEdit = pField ? pView->SearchEdit(pField) : NULL;

    if( pEdit != NULL ) {
        pView->SendMessage(UWM::CSEntry::ShowCapi, wParam, (LPARAM)pEdit);
    }

    if (bRefreshForLanguageChange) {
        CDEFormFile* pFormfile = pDoc->GetCurFormFile();
        pFormfile->RefreshAssociatedFieldText();
        pView->ResetGrids(true);
    }

    OnEngineRefresh(0, 0); //Refresh the screen.

    return 0;
}

//end SERPRO


void CMainFrame::OnUpdateInteractiveEditOptions(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CNPifFile* pPifFile = pDoc->GetPifFile();
    BOOL bEnable = FALSE;

    if( pPifFile != NULL )
    {
        if( pPifFile->GetInteractiveEditMode() == InteractiveEditMode::Ask )
            bEnable = TRUE;

        else if( ( pPifFile->GetInteractiveEditMode() != InteractiveEditMode::Off ) && !pPifFile->GetInteractiveEditDialogLocked() )
            bEnable = TRUE;
    }

    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnInteractiveEditOptions()
{
    intEdtDlg.DoModal();
}

void CMainFrame::OnUpdateInteractiveEdit(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    BOOL bEnable = ( ( pDoc->GetAppMode() == MODIFY_MODE ) && ( pDoc->GetPifFile()->GetInteractiveEditMode() != InteractiveEditMode::Off ) );
    pCmdUI->Enable(bEnable);
}

/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnInteractiveEdit()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnInteractiveEdit()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();
    CNPifFile* pPifFile = pRunDoc->GetPifFile();
    Application* pApp = pPifFile->GetApplication();

    if( pPifFile->GetInteractiveEditMode() == InteractiveEditMode::Off )
        return;

    bool bIgnore = false;

    if( pPifFile->GetInteractiveEditMode() == InteractiveEditMode::Ask  && pRunDoc->GetAppMode()==MODIFY_MODE ) {
        //Show the dialog
        if( intEdtDlg.DoModal() != IDOK )
            return;
        bShowIntDlg = false;
    }
    else { //we are not showing the dialog //get the stuff from piffile
        switch (pPifFile->GetInteractiveEditMode()) {
        case InteractiveEditMode::ErrMsg:
            intEdtDlg.m_iOption = 0;
            break;
        case InteractiveEditMode::Range:
            intEdtDlg.m_iOption = 1;
            break;
        case InteractiveEditMode::Both:
            intEdtDlg.m_iOption = 2;
            break;
        default :
            intEdtDlg.m_iOption = -1;
            break;
        }
    }

    if(intEdtDlg.m_iOption == -1){
        return;
    }

    CEntryrunView* pRView = GetRunView();
    CCaseView* pCView = GetCaseView();
    bool bAskWhenOutOfRange = pRunApl->GetSettings()->AskWhenOutOfRange();
    bool bAskWhenNotappl =  pRunApl->GetSettings()->AskWhenNotappl();

    pRunApl->SetInteractiveEdit(true);
    pRunDoc->SetIntEdit(true);

    bool bShowEndCaseMsgFlag = pApp->GetShowEndCaseMessage();
    pApp->SetShowEndCaseMessage(false);

    if(pRunDoc->GetAppMode() == MODIFY_MODE) { //For now //SAVY&&&

        if( intEdtDlg.m_iOption == 0 ) {//Only Messages
            pRunApl->GetSettings()->SetDontAskWhenOutOfRange();
            pRunApl->GetSettings()->SetDontAskWhenNotappl();
            pRunApl->SetStopOnOutOfRange(false);
        }
        else if( intEdtDlg.m_iOption == 1 ) {//Only Out of ranges
            pRunApl->GetSettings()->SetMustAskWhenOutOfRange();
            pRunApl->GetSettings()->SetMustAskWhenNotappl();
        }
        else if( intEdtDlg.m_iOption ==2 ) {//Both
            pRunApl->GetSettings()->SetMustAskWhenOutOfRange();
            pRunApl->GetSettings()->SetMustAskWhenNotappl();
        }

        while( true )
        {
            if( pRunDoc->GetCurField() == NULL )
                break;

            CDEItemBase* pItem = NULL;
            bool bBuildTree = false;

            if( pRunDoc->GetQModified() )
            {
                pApp->SetShowEndCaseMessage(bShowEndCaseMsgFlag);
                pRunApl->SetIgnoreWrite(false);
                bBuildTree = true;
            }

            else
            {
                pApp->SetShowEndCaseMessage(false);
                pRunApl->SetIgnoreWrite(true);
            }

            GetCaseView()->SetFocus(); //Common window to set focus to for receiving the key stroke
            //Get the current field and set data . //Problem in grids

            CDEField* pField = (CDEField*)pRunDoc->GetCurField();
            if(pField) {
                CDEBaseEdit* pEdit = GetRunView()->SearchEdit(pField);
                if(pEdit) {
                    GetRunView()->PutEditValInBuffers(pEdit);

                    CString sData;
                    pEdit->GetWindowText(sData);
                    pField->SetData(sData);
                }
            }

            pItem = pRunApl->AdvanceToEnd( false,TRUE );
            if(pItem){
                pRView->ChkFrmChangeNUpdate((CDEField*)pItem);
                if(pRunDoc->GetAppMode() == ADD_MODE)
                    pRView->ProcessFldAttrib((CDEField*)pItem);
                break;
            }

            if( pItem == NULL ) // try and go to next case
            {
                pRunDoc->SetQModified(FALSE);
                GetRunView()->ResetGrids();

                HTREEITEM hItem = pCView->GetTreeCtrl().GetSelectedItem();
                hItem = pCView->GetTreeCtrl().GetNextSiblingItem(hItem);

                if( hItem == NULL )
                {
                    OnStop();
                    break;
                }

                if( bBuildTree )
                {
                    CWaitCursor wait;
                    bBuildTree = false; // reset the flag

                    int iCase = pCView->GetSelectedCaseNumber();

                    pCView->BuildTree();

                    pCView->SetCaseNumber(iCase);
                    pCView->RestoreSelectPos();

                    hItem = pCView->GetTreeCtrl().GetSelectedItem();
                    hItem = pCView->GetTreeCtrl().GetNextSiblingItem(hItem);
                }

                MSG msg;
                if(::PeekMessage( &msg, GetCaseView()->GetSafeHwnd(), WM_KEYFIRST , WM_KEYLAST , PM_REMOVE)){
                    if(msg.wParam == VK_RETURN || msg.wParam == 78 || msg.wParam == VK_F11 ){ //78 Is 'N' ascii . kludge to eliminate
                        //the key down message passed to the window inspite of doing a remove in the peek message
                        bIgnore = true;
                    }
                    if(!bIgnore) {
                        if (AfxMessageBox (_T("Do you want to cancel interactive edit?"), MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDYES) {
                            hItem = NULL;
                        }
                    }
                    bIgnore = false;
                }

                if( hItem != NULL )
                {
                    NODEINFO* pNodeInfo = (NODEINFO*)pCView->GetTreeCtrl().GetItemData(hItem);

                    if( pNodeInfo->case_summary.IsPartial() )
                    {
                        AfxMessageBox(_T("Cannot enter a partially saved case. Stopping interactive edit."));
                        hItem = NULL;
                    }
                }

                if( hItem != NULL )
                    pCView->GetTreeCtrl().Select(hItem,TVGN_CARET);

                else
                {
                    OnStop();
                    break;
                }
            }
        }
    }

    pRunDoc->SetIntEdit(false);
    pRunApl->SetStopAdvance(false);

    if( bAskWhenOutOfRange)
        pRunApl->GetSettings()->SetMustAskWhenOutOfRange();
    else
        pRunApl->GetSettings()->SetDontAskWhenOutOfRange();

    if( bAskWhenNotappl )
        pRunApl->GetSettings()->SetMustAskWhenNotappl();
    else
        pRunApl->GetSettings()->SetDontAskWhenNotappl();

    pRunApl->SetStopOnOutOfRange(false);
    pRunApl->SetInteractiveEdit(false);
    pRunApl->SetIgnoreWrite(true);
    pApp->SetShowEndCaseMessage(bShowEndCaseMsgFlag);
}


void CMainFrame::OnViewRefusals()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    if( pDoc != nullptr )
    {
        CRunAplEntry* pRunAplEntry = pDoc->GetRunApl();

        if( pRunAplEntry != nullptr )
        {
            pRunAplEntry->ShowRefusalProcessor(CRunAplEntry::ShowRefusalProcessorAction::ShowRefusedValues, false);
            PostMessage(UWM::CSEntry::ShowCapi);
        }
    }
}

void CMainFrame::OnUpdateViewRefusals(CCmdUI* pCmdUI)
{
    bool field_has_refusals_not_shown = false;

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    if( pDoc != nullptr )
    {
        CRunAplEntry* pRunAplEntry = pDoc->GetRunApl();

        if( pRunAplEntry != nullptr )
            field_has_refusals_not_shown = pRunAplEntry->ShowRefusalProcessor(CRunAplEntry::ShowRefusalProcessorAction::ReturnWhetherRefusedValuesAreHidden, false);
    }

    pCmdUI->Enable(field_has_refusals_not_shown);
}


void CMainFrame::OnViewCheat()         // BMD 05 Mar 2002
{
    CEntryrunView* pView = GetRunView();

    pView->SendMessage(UWM::CSEntry::CheatKey);
}

void CMainFrame::OnUpdateViewCheat(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CEntryrunView* pView = GetRunView();

    if (pDoc->GetAppMode() == VERIFY_MODE) {
        pCmdUI->Enable();
        if (pView->GetCheatKey()) {
            pCmdUI->SetCheck(1);
        }
        else {
            pCmdUI->SetCheck(0);
        }
    }
    else {
        pCmdUI->Enable(FALSE);
    }
    return;
}



LRESULT CMainFrame::OnTreeItemWithNothingToDo(WPARAM wParam, LPARAM lParam)
{
    //When the user double click on an item with no action asociated,
    //the view lost the focus.

    CEntryrunDoc*   pDoc            = (CEntryrunDoc*) GetActiveDocument();
    CDEField *              pCurField       = pDoc ? (CDEField*) pDoc->GetCurField() : NULL;
    int                             iOcc            = pCurField ? pCurField->GetParent()->GetCurOccurrence() : 0;
    CEntryrunView*  pView           =  GetRunView();

    if(!pView || !IsWindow(pView->GetSafeHwnd())){
        return FALSE;
    }

    pView->GoToField(pCurField,iOcc);

    return TRUE;
}

//FABN Nov 11, 2002
LRESULT CMainFrame::OnRestoreEntryRunViewFocus(WPARAM wParam, LPARAM lParam)
{
    CEntryrunDoc*   pDoc            = (CEntryrunDoc*) GetActiveDocument();
    CDEField *      pCurField       = pDoc ? (CDEField*) pDoc->GetCurField() : NULL;
    int             iOcc            = pCurField ? pCurField->GetParent()->GetCurOccurrence() : 0;
    CEntryrunView*  pView           =  GetRunView();

    if(!pView || !IsWindow(pView->GetSafeHwnd())){
        return FALSE;
    }

    pView->GoToField(pCurField,iOcc);

    return TRUE;
}

//FABN Nov 11, 2002
LRESULT CMainFrame::OnPage1ChangeShowStatus(WPARAM wParam, LPARAM lParam)
{
    if (m_bPage1StatusChangeStopFocusChangeHack) {
        m_bPage1StatusChangeStopFocusChangeHack = false;
        return TRUE;
    }

    CEntryrunView*  pView           =  GetRunView();
    if(!pView || !IsWindow(pView->GetSafeHwnd())){
        return FALSE;
    }

    CEntryrunDoc*   pDoc        = (CEntryrunDoc*) GetActiveDocument();
    CDEField *      pCurField   = pDoc ? (CDEField*) pDoc->GetCurField()    : NULL;
    CDEBaseEdit*        pEdit       = pCurField ? pView->SearchEdit(pCurField)  : NULL;

    if(pEdit!=NULL){
        pEdit->SetFocus();
    } else {
        pView->SetFocus();
    }

    return TRUE;
}


//FABN Nov 4, 2002

LRESULT CMainFrame::OnGivenGoTo(WPARAM wParam, LPARAM lParam)
{
    CMsgParam*              pMsgParam               = (CMsgParam*) wParam;

    CDEField*               pSelectedField  = (CDEField*) pMsgParam->dwArrayParam.GetAt(0);

    int                             iSelectedOcc    = pMsgParam->iParam;

    if( pSelectedField && iSelectedOcc>0 ){
        CEntryrunView* pView = GetRunView();
        CWaitCursor wait;
        pView->GoToField(pSelectedField,iSelectedOcc);
    }

    if( pMsgParam && pMsgParam->bMustBeDestroyedAfterLastCatchMessage ){
        delete(pMsgParam);
        pMsgParam = NULL;
    }

    return TRUE;
}

LRESULT CMainFrame::OnSelectiveRefreshCaseTree(WPARAM wParam, LPARAM /*lParam*/)
{
    const std::vector<void*>& vpArray = *reinterpret_cast<std::vector<void*>*>(wParam);
    ASSERT(vpArray.size() == 3);

    std::unique_ptr<CArray<CDEItemBase*,CDEItemBase*>> pItemBaseArray;
    std::unique_ptr<CArray<CArray<int,int>*,CArray<int,int>*>> pOccsToRefreshArray;
    std::unique_ptr<std::vector<std::unique_ptr<CArray<int,int>>>> pOccsToRefreshArray_objects;

    if( vpArray[1] != nullptr )
    {
        const std::vector<CDEItemBase*>& aGroupArray = *reinterpret_cast<std::vector<CDEItemBase*>*>(vpArray[1]);
        pItemBaseArray = std::make_unique<CArray<CDEItemBase*,CDEItemBase*>>();
        VectorToCArray(aGroupArray, *pItemBaseArray);
    }

    if( vpArray[2] != nullptr )
    {
        const std::vector<std::vector<int>>& aOccsArray = *reinterpret_cast<const std::vector<std::vector<int>>*>(vpArray[2]);
        pOccsToRefreshArray = std::make_unique<CArray<CArray<int,int>*,CArray<int,int>*>>();
        pOccsToRefreshArray_objects = std::make_unique<std::vector<std::unique_ptr<CArray<int,int>>>>();

        for( const std::vector<int>& occs_array : aOccsArray )
        {
            CArray<int,int>* pOccsToRefreshArray_object = pOccsToRefreshArray_objects->emplace_back(std::make_unique<CArray<int,int>>()).get();
            VectorToCArray(occs_array, *pOccsToRefreshArray_object);
            pOccsToRefreshArray->Add(pOccsToRefreshArray_object);
        }
    }

    CMsgParam msgParam;
    msgParam.dwArrayParam.Add( (DWORD) vpArray[0] );
    msgParam.dwArrayParam.Add( (DWORD) pItemBaseArray.get() );
    msgParam.dwArrayParam.Add( (DWORD) pOccsToRefreshArray.get() );

    SendMessage(UWM::CaseTree::RefreshCaseTree, (WPARAM)&msgParam, -2);

    return TRUE;
}



//FABN Nov 6, 2002
LRESULT CMainFrame::OnRefreshCaseTree(WPARAM wParam, LPARAM lParam)
{
    //FABN Feb 20, 2003 : When the tree doesn't exist and exist cur field => the tree must be created
    CCaseTree*    pCaseTree     = GetCaseTree();

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();

    bool  bShowCaseTree = pDoc->GetPifFile()->GetApplication()->GetShowCaseTree();
    if(!bShowCaseTree) {
        return TRUE;
    }

    CMsgParam& msgParam = *((CMsgParam*) wParam);

    CDEItemBase* pItem=NULL;// RHF Jun 20, 2003 Set as NULL

    switch( lParam ){
    case 0  : { pItem = (CDEItemBase*) msgParam.dwArrayParam.GetAt(0);                              } break;
    case -2 : { pItem = (CDEItemBase*) msgParam.dwArrayParam.GetAt(0);                              } break;
    default : { ASSERT( false ); /*you must provide a way to extract the pItem from the message*/   } break;
    }

    if( pItem!=NULL && pCaseTree==NULL && m_bTreeEnable ){
        SendMessage(UWM::CaseTree::CreateCaseTree);
    }


    CLeftPropSheet* pPropSheet          = m_pLeftView   ? m_pLeftView->GetPropSheet()   : NULL;
    int             iActivePropPageIdx  = pPropSheet    ? pPropSheet->GetActiveIndex()  : -1;
    if(iActivePropPageIdx!=1){
        return FALSE;
    }

    if(!pCaseTree || !IsWindow(pCaseTree->GetSafeHwnd())){
        return FALSE;
    }

    return pCaseTree->SendMessage(UWM::CaseTree::Refresh, wParam, lParam);
}

//FABN Nov 8, 2002
LRESULT CMainFrame::OnGoToNode(WPARAM wParam,LPARAM lParam)
{
    CMsgParam*      pMsgParam       = (CMsgParam*) wParam;

    int                     iNodeIdx        = pMsgParam->iParam;

    if( pMsgParam && pMsgParam->bMustBeDestroyedAfterLastCatchMessage ){
        delete(pMsgParam);
        pMsgParam = NULL;
    }

    CEntryrunDoc*           pDoc                    = (CEntryrunDoc*) GetActiveDocument();
    CRunAplEntry*           pRunAplEntry    =       pDoc->GetRunApl();
    if( pRunAplEntry->SetStopNode(iNodeIdx) ){
        CDEItemBase* pItem = pRunAplEntry->AdvanceToEnd(false, true, iNodeIdx);

        ASSERT(!pItem || pItem->GetItemType() == CDEFormBase::Field);

        CDEField*       pCurField       = pItem ? (CDEField*) pItem : NULL;
        int                     iCurOcc                 = pCurField ? pCurField->GetParent()->GetCurOccurrence() : -1;

        if(!pCurField){
            m_pLeftView->DeleteCaseTree();
        }


        //Refresh the view (the view will take the focus)
        CEntryrunView* pView = GetRunView();
        CWaitCursor wait;
        pView->ResetGrids();
        pView->ResetForm();

        // RHF INIC Jul 21, 2003
        if( pCurField == NULL ) {
            if(pDoc->GetAppMode() != ADD_MODE)  {
                pDoc->SetQModified(FALSE);
                pView->ProcessModifyMode();
            }

            return 0l;
        }
        // RHF END Jul 21, 2003
        pView->GoToField(pCurField,iCurOcc);
    }

    return TRUE;
}

//FABN Nov 7, 2002
LRESULT CMainFrame::OnDeleteCaseTree(WPARAM wParam,LPARAM lParam)
{
    m_pLeftView->DeleteCaseTree();
    return TRUE;
}

LRESULT CMainFrame::OnCreateCaseTree(WPARAM wParam,LPARAM lParam)
{
    m_pLeftView->CreateCaseTree();
    return TRUE;
}

//FABN Nov 20, 2002
LRESULT CMainFrame::OnUnknownKey(WPARAM wParam,LPARAM lParam)
{
    //This message has been received from the case tree, when the user press any key that the CCaseTree don't have
    //any functionality. So, here the MainFrame can take appropiate actions.

    #ifdef _DEBUG
        TRACE(_T("CMainFrame::OnUnknownKey\n"));
    #endif

    CArray<DWORD,DWORD>*  pArray = (CArray<DWORD,DWORD>*) wParam;

    ASSERT(pArray->GetSize()==6);

    //params from the OnKeyDown call
    UINT nChar  = pArray->GetAt(0);

    //and the virtual keys status when the message was fired
    bool bCtrl  = pArray->GetAt(3) != 0;
    bool bUsedKey = true;

    //THE KEY WAS A CTRL+KEY COMBINATION
    if( bCtrl ){

        switch(nChar){

                case _T('T') : OnToggleNames();     // smg: added this blk 21 may 03
                                        break;
        case _T('U') : {
                        OnFullscreen();
                        SendMessage(UWM::CaseTree::RestoreEntryRunViewFocus, 0, 0);

                    } break;
        case _T('G') : {
            CWnd* pWnd = GetFocus();
            if(pWnd) {
                if(pWnd->IsKindOf(RUNTIME_CLASS(CEntryrunView)) || pWnd->IsKindOf(RUNTIME_CLASS(CEdit))){
                    //set focus to tree view
                    if(m_pLeftView) {
                        m_pLeftView->GetPropSheet()->GetActivePage()->SetFocus();
                    }
                }
                else {
                    GetRunView()->SetFocus();
                }

            }

                   } break;

        default : { bUsedKey = false; } break;

        }

    } else {
        bUsedKey = false;
    }

    //FABN Jan 15, 2003
    if(!bUsedKey){
        SendMessage(UWM::CSEntry::AcceleratorKey, wParam, lParam);
    }
    return TRUE;
}

LRESULT CMainFrame::OnAcceleratorKey(WPARAM wParam, LPARAM lParam)
{
    #ifdef _DEBUG
        TRACE(_T("CMainFrame::OnAcceleratorKey\n"));
    #endif

    CArray<DWORD,DWORD>*  pArray = (CArray<DWORD,DWORD>*) wParam;

    ASSERT(pArray->GetSize()==6);

    //params from the original OnKeyDown call
    UINT nChar          = pArray->GetAt(0);
    UINT nRepCnt        = pArray->GetAt(1);
    UINT nFlags         = pArray->GetAt(2);

    //and the virtual keys status when the message was fired
    bool bCtrl          = pArray->GetAt(3) != 0;
    bool bShift         = pArray->GetAt(4) != 0;
    bool bAlt           = pArray->GetAt(5) != 0;


    #ifdef _DEBUG
        TRACE(_T("CMainFrame::OnAcceleratorKey : nChar=%d, nRepCnt=%d, nFlags=%d, bCtrl=%d, bShift=%d, bAlt=%d\n"), nChar, nRepCnt, nFlags, bCtrl, bShift, bAlt );
    #endif


    int     iCmdID;
    bool    bEnabled    = false;

    GetCmd( nChar, bCtrl, bShift, bAlt,  &iCmdID, &bEnabled);
    if(bCtrl && nChar == 'G') {
        CWnd* pWnd = GetFocus();
        if(pWnd) {
            if(pWnd->IsKindOf(RUNTIME_CLASS(CEntryrunView)) || pWnd->IsKindOf(RUNTIME_CLASS(CEdit))){
                //set focus to tree view
                if(m_pLeftView) {
                    m_pLeftView->GetPropSheet()->GetActivePage()->SetFocus();
                }
            }
            else {
                GetRunView()->PostMessage(WM_SETFOCUS);
                //return TRUE;
                /*CEntryrunDoc*   pDoc      = (CEntryrunDoc*) GetActiveDocument();
                CEntryrunView*  pView     = GetRunView();
                CDEField*       pCurField = pDoc ? (CDEField *) pDoc->GetCurField() : NULL;
                CDEBaseEdit*        pCurEdit  = pCurField ? pView->SearchEdit(pCurField) : NULL;
                if(pCurEdit){
                    //pCurEdit->SetFocus();
                    pCurEdit->SendMessage( UWM::CSEntry::SimulatedKeyDown, wParam, lParam );
                }*/
            }

        }

    }
    if( iCmdID!=-1 ){

        if( bEnabled ){


                #ifdef _DEBUG
                    TRACE(_T("Sending WM_COMMAND message, with iCmdID = %d"), iCmdID);
                #endif

                SendMessage( WM_COMMAND, iCmdID );

        }

    } else {    //The key is not in the accelerator table, maybe is listened in CDEBaseEdit::OnKeyDown

        if( nChar != VK_DELETE  ){

            CEntryrunDoc*   pDoc      = (CEntryrunDoc*) GetActiveDocument();
            CEntryrunView*  pView     = GetRunView();
            CDEField*       pCurField = pDoc ? (CDEField *) pDoc->GetCurField() : NULL;
            CDEBaseEdit*        pCurEdit  = pCurField ? pView->SearchEdit(pCurField) : NULL;
            if(pCurEdit && pView == GetActiveView()){
                pCurEdit->SendMessage(UWM::CSEntry::SimulatedKeyDown, wParam, lParam);
            }
        }
    }

    pArray->RemoveAll();
    delete(pArray);
    pArray = NULL;

    return TRUE;
}

//FABN March 3, 2003
LRESULT CMainFrame::OnCasesTreeFocus(WPARAM wParam, LPARAM lParam)
{
    CEntryrunDoc* pDoc          = (CEntryrunDoc*) GetActiveDocument();
    CRunAplEntry* pRunAplEntry  = pDoc ? pDoc->GetRunApl() : NULL;
    if(pRunAplEntry && pRunAplEntry->GetAppMode()==CRUNAPL_NONE ){
        CCaseView*  pCaseView = GetCaseView();
        if(pCaseView){
            pCaseView->SetFocus();
        }
    }

    return TRUE;
}


LONG CMainFrame::OnFieldBehavior(WPARAM wParam, LPARAM lParam) {
    CEntryrunView*  pView=GetRunView();
    CEntryrunDoc*   pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*   pRunApl = NULL;

    if( pView == NULL || pDoc == NULL || (pRunApl = pDoc->GetRunApl()) == NULL  )
        return 0;

    CDEField*   pField=pRunApl->GetDeField( (DEFLD*)wParam );

    if( pField != NULL ) {
        FieldBehavior   eBehavior=(FieldBehavior) lParam;

        if( eBehavior == AsProtected ) {
            pField->IsProtected( true );
        }
        else if( eBehavior == AsAutoSkip ) {
            pField->IsProtected( false );
            pField->IsEnterKeyRequired( false );
        }
        else if( eBehavior == AsEnter ) {
            pField->IsProtected( false );
            pField->IsEnterKeyRequired( true );
        }
        else
            ASSERT(0);
    }

    return 0L;
}


LONG CMainFrame::OnFieldVisibility(WPARAM wParam, LPARAM lParam) {
    CEntryrunView*  pView=GetRunView();
    CEntryrunDoc*   pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*   pRunApl = NULL;

    if( pView == NULL || pDoc == NULL || (pRunApl = pDoc->GetRunApl()) == NULL  )
        return 0;

    CDEField*   pField=pRunApl->GetDeField( (DEFLD*)wParam );
    //CDEBaseEdit*    pEdit = pField ? pView->SearchEdit(pField) : NULL;

    if( pField != NULL ) {
        bool    bVisible=(lParam==1);

        pField->IsHidden( !bVisible );
    }

    return 0L;
}


void CMainFrame::OnLanguage()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = ( pDoc != nullptr ) ? pDoc->GetRunApl() : nullptr;

    if( pRunApl == nullptr )
        return;

    bool language_changed = false;

    if( UseHtmlDialogs() )
    {
        language_changed = pRunApl->ChangeLanguage();
    }

    else
    {
        std::vector<Language> languages = pRunApl->GetLanguages();

        std::vector<std::vector<CString>*> aData;
        std::vector<bool> baSelections;

        for( const Language& language : languages )
        {
            aData.emplace_back(new std::vector<CString>{ WS2CS(language.GetLabel().empty() ? language.GetName() :
                                                                                             language.GetLabel()) });
        }

        baSelections.resize(aData.size());

        CSelectListCtrlOptions cOptions;
        cOptions.m_paData = &aData;
        cOptions.m_pbaSelections = &baSelections;

        cOptions.m_bUseTitle = true;
        cOptions.m_csTitle = WS2CS(MGF::GetMessageText(MGF::SelectLanguageTitle));
        cOptions.m_iMinMark = 0;
        cOptions.m_iMaxMark = 1;
        cOptions.m_bUseColTitle = false;
        cOptions.m_bHighLightFirst = true;

        CSelectDlg selectDlg(AfxGetMainWnd());

        int iLanguage = -1;

        if( selectDlg.DoModal(&cOptions) == IDOK )
        {
            for( std::vector<byte>::size_type i = 0; i < baSelections.size(); i++ )
            {
                if( baSelections[i] )
                {
                    iLanguage = i;
                    break;
                }
            }
        }

        for( std::vector<std::vector<CString>*>::size_type i = 0; i < aData.size(); i++ )
            delete aData[i];

        if( iLanguage >= 0 )
        {
            pRunApl->SetCurrentLanguage(languages[iLanguage].GetName());

            // 20130306 adding the OnChangeLanguage function (in logic)
            if( pRunApl->HasSpecialFunction(SpecialFunction::OnChangeLanguage) )
                pRunApl->ExecSpecialFunction(SpecialFunction::OnChangeLanguage);

            language_changed = true;
        }
    }

    if( language_changed )
    {
        CDEField* pCurField = (CDEField*)pDoc->GetCurField();
        CEntryrunView* pView = GetRunView();

        // copied from userbar stuff...
        if( pRunApl->HasSomeRequest() )
        {
            pView->PostMessage(UWM::CSEntry::MoveToField);
        }

        else // this kludge makes it so that if the user function called errmsg, the cursor returns to its rightful position
        {
            OnEngineShowCapi(0,1); // 20140113 in case the QSF text has changed (because of replacement parameters updated in OnChangeLanguage)
            pView->ShowLabels(pCurField);
            pView->GoToFld(pCurField);
        }

        PostMessage(WM_IMSA_CSENTRY_REFRESH_DATA);
    }
}

void CMainFrame::OnUpdateLanguage(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl;
    BOOL bEnable = FALSE;

    if( pDoc != NULL && ( pRunApl = pDoc->GetRunApl() ) != NULL && pDoc->GetAppMode() != NO_MODE )
        bEnable = ( pRunApl->GetLanguages(false).size() > 1 );

    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnCapiToggle() {
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl;
    CDEField*     pCurField;
    if(pDoc && (pRunApl=pDoc->GetRunApl()) != NULL && (pCurField = (CDEField*)pDoc->GetCurField()) != NULL ) {
        pRunApl->ToggleCapi( pCurField->GetSymbol() );

        SendMessage(UWM::CSEntry::ShowCapi, 0, 0); // Refresh
    }
}

void CMainFrame::OnUpdateCapiToggle(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    if( (pDoc->GetAppMode() == MODIFY_MODE || pDoc->GetAppMode() == ADD_MODE || pDoc->GetAppMode() == VERIFY_MODE ) &&
        pDoc->GetCurField() != NULL )
        pCmdUI->Enable( TRUE );
    else
        pCmdUI->Enable( FALSE);
}

void CMainFrame::OnCapiToggleAllVars() {
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl;
    if(pDoc && (pRunApl=pDoc->GetRunApl()) != NULL && pDoc->GetCurField() != NULL ) {
        pRunApl->ToggleCapi( -1 ); // All Vars

        SendMessage(UWM::CSEntry::ShowCapi, 0, 0); // Refresh
    }
}

void CMainFrame::OnUpdateCapiToggleAllVars(CCmdUI* pCmdUI)
{
    OnUpdateCapiToggle( pCmdUI );
}


void CMainFrame::OnPartialSaveCase()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl;

    if( pDoc != NULL && ( pRunApl = pDoc->GetRunApl() ) != NULL && pDoc->GetAppMode() != NO_MODE )
    {
        bool bSaved = pRunApl->PartialSaveCase(pDoc->GetAppMode(),m_bPartialSaveClearSkipped);

        if( bSaved )
            pDoc->SetQModified(FALSE);

        if( !m_bPartialSaveFromApp || !bSaved )
        {
            // display a message regarding the success
            int iMessageNumber = bSaved ? MGF::PartialSaveSuccess : MGF::PartialSaveFailure;
            AfxMessageBox(MGF::GetMessageText(iMessageNumber).c_str());
        }

        m_bPartialSaveFromApp = bSaved;
    }

    else
        m_bPartialSaveFromApp = false;
}


/////////////////////////////////////////////////////////////////////////////////
//
// void CMainFrame::OnUpdatePartialSaveCase(CCmdUI* pCmdUI)
//
// this tells me when i can do a save; they can only save if all the IDs have
// been keyed for that level
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdatePartialSaveCase(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl;

    BOOL bEnable = FALSE;

    if( pDoc && pDoc->GetQModified() && (pRunApl=pDoc->GetRunApl()) != NULL && pDoc->GetCurField() != NULL )
    {
        bool bAllowPartial = pDoc->GetPifFile()->GetApplication()->GetPartialSave();

        if( bAllowPartial && pDoc->GetAppMode() != NO_MODE )
        {
            int iCurrentLevel = pRunApl->GetCurrentLevel();

            if( iCurrentLevel >= 2 || pRunApl->QidReady(iCurrentLevel) )
                bEnable = TRUE;
        }
    }

    pCmdUI->Enable( bEnable );
}

void CMainFrame::OnSynchronize()
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetActiveDocument();

    if (pDoc && pDoc->GetAppMode() == NO_MODE) {
        const AppSyncParameters& syncParams = pDoc->GetPifFile()->GetApplication()->GetSyncParameters();
        CRunAplEntry* pRunApl = pDoc->GetRunApl();
        if (pRunApl->RunSync(syncParams)) {

            // update the case listing
            BuildKeyArray();
            GetCaseView()->BuildTree();

            AfxMessageBox(_T("Synchronization complete"), MB_OK | MB_ICONINFORMATION);
        }
    }
}

void CMainFrame::OnUpdateSynchronize(CCmdUI * pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*) GetActiveDocument();
    BOOL bEnable = FALSE;

    if (pDoc && pDoc->GetPifFile() && pDoc->GetPifFile()->GetApplication() && pDoc->GetAppMode() == NO_MODE) {
        const AppSyncParameters& syncParams = pDoc->GetPifFile()->GetApplication()->GetSyncParameters();
        if (!syncParams.server.empty()) {
            bEnable = TRUE;
        }
    }
    pCmdUI->Enable(bEnable);
}


LONG CMainFrame::OnRefreshSelected(WPARAM wParam, LPARAM /*lParam*/)
{
    CWnd* pFieldWnd = (CWnd*)wParam;

    CString csMarked;
    pFieldWnd->GetWindowText(csMarked);

    CEntryrunDoc*   pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry*   pRunApl;
    CDEField*       pCurField;
    CEntryrunView*  pView=GetRunView();

    if(pDoc && pView && (pRunApl=pDoc->GetRunApl()) != NULL && (pCurField = (CDEField*)pDoc->GetCurField()) != NULL ) {
        CCapi* pCapi=pRunApl->GetCapi();

        if( pCapi->GetAroundField() != pFieldWnd ) {
            return 0; // RHF Jan 20, 2003
        }

        pCapi->UpdateSelection(csMarked);
    }

    return 0;
}



// this function is called by the user bar to execute a function tied to a button click or field enter
// there are three goals: 1) execute the function; 2) return the focus to the form and then;
// 3) update the position of the cursor in the case that the function called had a move statement

LONG CMainFrame::OnUserbarUpdate(WPARAM wParam, LPARAM lParam) // 20100415
{
    CEntryrunDoc* pDoc = GetDocument();

    if( lParam < 0 ) // an easy way to get data entry to stop in the case that the user's function had a stop command
    {
        pDoc->SetCurField(NULL); // this and the next line will keep OnStop from asking about a partial save
        pDoc->SetQModified(false);
        OnStop();
        return 0;
    }

    Userbar::Action* userbar_action = reinterpret_cast<Userbar::Action*>(wParam);

    // run a control action
    if( std::holds_alternative<Userbar::ControlAction>(*userbar_action) )
    {
        CDEField* pField = (CDEField*)pDoc->GetCurField();
        CDEBaseEdit* pEdit = ( pField != nullptr ) ? GetRunView()->SearchEdit(pField) : nullptr;

        if( pEdit == nullptr )
        {
            // the user is currently not on a field somehow, we shouldn't get here
            ASSERT(false);
            return 0;
        }

        switch( std::get<Userbar::ControlAction>(*userbar_action) )
        {
            case Userbar::ControlAction::Next:
                GetRunView()->PostMessage(UWM::CSEntry::ChangeEdit, VK_RIGHT, (LPARAM)pEdit);
                break;
            case Userbar::ControlAction::Previous:
                GetRunView()->PostMessage(UWM::CSEntry::ChangeEdit, VK_LEFT, (LPARAM)pEdit);
                break;
            case Userbar::ControlAction::Advance:
                OnAdvtoend();
                break;
            case Userbar::ControlAction::Note:
                DoNote(false);
                break;
            case Userbar::ControlAction::Language:
                OnLanguage();
                break;
            case Userbar::ControlAction::Save:
                OnPartialSaveCase();
                m_bPartialSaveFromApp = false; // this will make sure that the save dialog box appears each time
                break;
            case Userbar::ControlAction::Help:
                GetRunView()->PostMessage(UWM::CSEntry::ChangeEdit, VK_F2, (LPARAM)pEdit);
                break;
            case Userbar::ControlAction::InsertLevelOcc:
                OnInsertNode();
                break;
            case Userbar::ControlAction::AddLevelOcc:
                OnAddNode();
                break;
            case Userbar::ControlAction::DeleteLevelOcc:
                OnDeleteNode();
                break;
            case Userbar::ControlAction::InsertGroupOcc:
                OnInsertGroupocc();
                break;
            case Userbar::ControlAction::InsertGroupOccAfter:
                OnInsertAfterOcc();
                break;
            case Userbar::ControlAction::DeleteGroupOcc:
                OnDeleteGrpocc();
                break;
            case Userbar::ControlAction::SortGroupOcc:
                OnSortgrpocc();
                break;
            case Userbar::ControlAction::PreviousForm:
                OnPrevScreen();
                break;
            case Userbar::ControlAction::NextForm:
                OnNextScreen();
                break;
            case Userbar::ControlAction::EndGroupOcc:
                OnNextGroupOcc();
                break;
            case Userbar::ControlAction::EndGroup:
                OnNextGroup();
                break;
            case Userbar::ControlAction::EndLevelOcc:
                OnNextLevelOcc();
                break;
            case Userbar::ControlAction::EndLevel:
                OnNextLevel();
                break;
            case Userbar::ControlAction::FullScreen:
                OnFullscreen();
                SendMessage(UWM::CaseTree::RestoreEntryRunViewFocus);
                break;
            case Userbar::ControlAction::ToggleResponses:
                OnCapiToggle();
                SendMessage(UWM::CaseTree::RestoreEntryRunViewFocus);
                break;
            case Userbar::ControlAction::ToggleAllResponses:
                OnCapiToggleAllVars();
                SendMessage(UWM::CaseTree::RestoreEntryRunViewFocus);
                break;
        }
    }

    // call a user defined function
    else
    {
        CRunAplEntry* pRunApl = (CRunAplEntry*)pDoc->GetRunApl();
        CDEField* pCurField = (CDEField*)pDoc->GetCurField();
        CEntryrunView* pView = GetRunView();

        CDEBaseEdit* pEdit = pView->SearchEdit(pCurField); // 20130705 for use later
        CString sPreData = pCurField->GetData();

        pRunApl->ExecuteCallbackUserFunction(pCurField->GetSymbol(), *std::get<std::unique_ptr<UserFunctionArgumentEvaluator>>(*userbar_action));

        if( pRunApl->HasSomeRequest() )
        {
            pRunApl->SetProgressForPreEntrySkip(); // 20130415
            pView->PostMessage(UWM::CSEntry::MoveToField);
        }

        else // this kludge makes it so that if the user function called errmsg, the cursor returns to its rightful position
        {
            pView->GoToFld(pCurField);
        }


        // 20130705 if the function called by the userbar modified the current field, then turn the modified flag off, which, when refresh data is called, will cause the field to update to the new logic-assigned value
        CString sPostData = pRunApl->GetVal(pCurField);

        if( sPreData.Compare(sPostData) )// || ( sPreText.Compare(sPostData) && pEdit->GetModifiedFlag() ) )
            pEdit->SetModifiedFlag(false);

        // unfortunately, the above check isn't perfect; for example, if the the userbar modifies the current field, then the user modifies the value and clicks on the button again, then the value won't change (because
        // the pre-data equals the post-data), but i can't think of a way to get around this

        PostMessage(WM_IMSA_CSENTRY_REFRESH_DATA);
    }

    return 0;
}



LONG CMainFrame::OnSetMessageOverrides(WPARAM wParam, LPARAM /*lParam*/) // 20100518
{
    const MessageOverrides& message_overrides = *((const MessageOverrides*)wParam);

    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    pDoc->SetMessageOverrides(message_overrides);

    return 0;
}

LONG CMainFrame::OnUsingOperatorControlledMessages(WPARAM wParam, LPARAM lParam)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = ( pDoc != nullptr ) ? pDoc->GetRunApl() : nullptr;
    bool using_operator_controlled_messages = false;

    if( pRunApl != nullptr )
    {
        const auto& message_overrides = pDoc->GetMessageOverrides();

        using_operator_controlled_messages = message_overrides.ForceOperatorControlled() ||
                                             ( !pRunApl->IsPathOn() && !message_overrides.ForceSystemControlled() );
    }

    return using_operator_controlled_messages ? 1 : 0;
}


LONG CMainFrame::OnGetUserFonts(WPARAM wParam, LPARAM /*lParam*/) // 20100621
{
    UserDefinedFonts** user_defined_fonts = reinterpret_cast<UserDefinedFonts**>(wParam);
    *user_defined_fonts = &m_userFonts;
    return 1;
}



// RHF END Nov 21, 2002

//FABN Jan 15, 2003
void CMainFrame::AddToAccelArrays( UINT uiMenuID, UINT nChar, bool bCtrl, bool bShift, bool bAlt)
{
    m_menuCmdIDArray.Add       ( uiMenuID  );
    m_menuCmdCharAccelArray.Add( nChar     );

    CString csFlag = _T("");
    if( bCtrl ){
        csFlag = csFlag + _T("1");
    } else {
        csFlag = csFlag + _T("0");
    }
    if( bShift ){
        csFlag = csFlag + _T("1");
    } else {
        csFlag = csFlag + _T("0");
    }
    if( bAlt ){
        csFlag = csFlag + _T("1");
    } else {
        csFlag = csFlag + _T("0");
    }

    m_menuCmdFlagArray.Add( csFlag );
}

void CMainFrame::GetCmd( UINT nChar, bool bCtrl, bool bShift, bool bAlt, int* iCmdID, bool* bEnabled)
{
    CString csFlag = _T("");
    if( bCtrl ){
        csFlag = csFlag + _T("1");
    } else {
        csFlag = csFlag + _T("0");
    }
    if( bShift ){
        csFlag = csFlag + _T("1");
    } else {
        csFlag = csFlag + _T("0");
    }
    if( bAlt ){
        csFlag = csFlag + _T("1");
    } else {
        csFlag = csFlag + _T("0");
    }

    int iIdx = -1;
    int iSize = m_menuCmdCharAccelArray.GetSize();
    for( int i=0; iIdx==-1 && i<iSize; i++){
        if( m_menuCmdCharAccelArray.GetAt(i)==nChar && m_menuCmdFlagArray.GetAt(i)==csFlag ){
            iIdx = i;
        }
    }

    if(iIdx==-1){
        *iCmdID     = -1;
        *bEnabled   = false;
        return;
    }

    *iCmdID   = m_menuCmdIDArray.GetAt( iIdx );
    *bEnabled = iIdx>=0 ? true : false;
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnToggleCaseTree()
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnToggleCaseTree()
{
    CEntryrunDoc* pDoc          = (CEntryrunDoc*) GetActiveDocument();
    bool  bShowCaseTree = pDoc->GetPifFile()->GetApplication()->GetShowCaseTree();

    bool          bEnableTree   = pDoc && (pDoc->GetAppMode()==MODIFY_MODE || pDoc->GetAppMode()==ADD_MODE || pDoc->GetAppMode()==VERIFY_MODE );
    if( bEnableTree && bShowCaseTree){
        int iCurTabSel = m_pLeftView->GetPropSheet()->GetTabControl()->GetCurSel();
        int iSel = iCurTabSel ? 0 : 1;
        m_pLeftView->GetPropSheet()->GetTabControl()->SetCurSel(iSel);
        m_pLeftView->GetPropSheet()->SetActivePage(iSel);
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//      void CMainFrame::OnUpdateToggleCaseTree( CCmdUI* pCmdUI )
//
/////////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateToggleCaseTree( CCmdUI* pCmdUI )
{
    CEntryrunDoc* pDoc      = (CEntryrunDoc*) GetActiveDocument();
        if(!pDoc->GetPifFile()) {
                pCmdUI->Enable(FALSE);
        return;
        }

    bool  bShowCaseTree = pDoc->GetPifFile()->GetApplication()->GetShowCaseTree();
    if(!bShowCaseTree) {
        pCmdUI->Enable(FALSE);
        return;
    }

    bool          bEnable   = pDoc && (pDoc->GetAppMode()==MODIFY_MODE || pDoc->GetAppMode()==ADD_MODE || pDoc->GetAppMode()==VERIFY_MODE );
    pCmdUI->Enable( bEnable );
    if(bEnable){
        int iCurTabSel = m_pLeftView->GetPropSheet()->GetTabControl()->GetCurSel();
        pCmdUI->SetCheck( iCurTabSel ? 1 : 0 );
    } else {
        pCmdUI->SetCheck( 0 );
    }
}


LRESULT CMainFrame::OnRecalcLeftLayout(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if(m_pLeftView){
        m_pLeftView->DummyMove();
    }

    return TRUE;
}

void CMainFrame::ChangeViewCaseStatus(CaseIterationCaseStatus eStatus)
{
    CCaseView* pCaseView = GetCaseView();
    CTreeCtrl& caseTree = pCaseView->GetTreeCtrl();
    HTREEITEM hSelItem = caseTree.GetSelectedItem();
    int iCase = -1;

    if( hSelItem != NULL )
    {
        NODEINFO* pNodeInfo = (NODEINFO*)caseTree.GetItemData(hSelItem);
        iCase = pNodeInfo->case_number;
    }

    m_eCaseStatusToShow = eStatus;
    GetCaseView()->BuildTree();

    if( iCase >= 0 )
    {
        pCaseView->SetCaseNumber(iCase);
        pCaseView->RestoreSelectPos();
    }
}

void CMainFrame::OnViewAll()
{
    ChangeViewCaseStatus(m_eCaseStatusToShow = CaseIterationCaseStatus::All);
}

void CMainFrame::OnViewNotDeleted()
{
    ChangeViewCaseStatus(m_eCaseStatusToShow = CaseIterationCaseStatus::NotDeletedOnly);
}

void CMainFrame::OnViewDuplicate()
{
    ChangeViewCaseStatus(m_eCaseStatusToShow = CaseIterationCaseStatus::DuplicatesOnly);
}

void CMainFrame::OnViewPartial()
{
    ChangeViewCaseStatus(m_eCaseStatusToShow = CaseIterationCaseStatus::PartialsOnly);
}

void CMainFrame::OnUpdateViewAll(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    BOOL bEnable = ( pDoc != NULL && pDoc->GetAppMode() == NO_MODE && DataRepositoryHelpers::TypeSupportsUndeletes(m_eRepoType) );

    pCmdUI->Enable(bEnable);
    pCmdUI->SetCheck(m_eCaseStatusToShow == CaseIterationCaseStatus::All);
}

void CMainFrame::OnUpdateViewNotDeleted(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    BOOL bEnable = (pDoc != NULL && pDoc->GetAppMode() == NO_MODE);

    pCmdUI->Enable(bEnable);
    pCmdUI->SetCheck(m_eCaseStatusToShow == CaseIterationCaseStatus::NotDeletedOnly);
}

void CMainFrame::OnUpdateViewDuplicate(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    BOOL bEnable = ( pDoc != NULL && pDoc->GetAppMode() == NO_MODE && DataRepositoryHelpers::TypeSupportsDuplicates(m_eRepoType) );

    pCmdUI->Enable(bEnable);
    pCmdUI->SetCheck(m_eCaseStatusToShow == CaseIterationCaseStatus::DuplicatesOnly);
}

void CMainFrame::OnUpdateViewPartial(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    BOOL bEnable = (pDoc != NULL && pDoc->GetAppMode() == NO_MODE);

    pCmdUI->Enable(bEnable);
    pCmdUI->SetCheck(m_eCaseStatusToShow == CaseIterationCaseStatus::PartialsOnly);
}

void CMainFrame::OnUpdateNote(CCmdUI* pCmdUI)
{
    CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
    BOOL bEnable = ( pDoc->GetAppMode() != NO_MODE );
    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnFieldNote()
{
    DoNote(false);
}

void CMainFrame::OnCaseNote()
{
    DoNote(true);
}

void CMainFrame::OnReviewNotes()
{
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetActiveDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    std::shared_ptr<const CaseItemReference> field_to_goto = pRunApl->ReviewNotes();

    if( field_to_goto != nullptr )
        GetRunView()->GoToField(*field_to_goto);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CMainFrame::OnSetSequential(WPARAM wParam, LPARAM lParam)
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::OnSetSequential(WPARAM wParam, LPARAM lParam)
{
    CDEField* pField = (CDEField*)wParam;
    //Get the field that is current
    ///get the pEdit and set window text
    CDEBaseEdit *pEdit = GetRunView()->SearchEdit(pField);
    if(pEdit){
        pEdit->SetWindowText(pField->GetData());
    }
    return 0l;

}

/////////////////////////////////////////////////////////////////////////////////
//
//      LONG CMainFrame::OnSetCapiText(WPARAM wParam, LPARAM lParam)
//
// revised csc 2/10/2004
//
/////////////////////////////////////////////////////////////////////////////////
LONG CMainFrame::OnSetCapiText(WPARAM wParam, LPARAM lParam)
{
    QSFView* qsf_view = GetQTxtView();
    auto capi_text = (const CString*)wParam;
    auto color = (const COLORREF*)lParam;

    qsf_view->SetText(( capi_text != nullptr ) ? *capi_text : CString(),
                      ( color != nullptr ) ? std::make_optional(PortableColor::FromCOLORREF(*color)) : std::nullopt);

    return 0;
}


LRESULT CMainFrame::OnGetWindowHeight(WPARAM wParam, LPARAM /*lParam*/)
{
    unsigned& height = *reinterpret_cast<unsigned*>(wParam);

    int cyCur;
    int cyMin;
    m_wndCapiSplitter.GetRowInfo(0, cyCur, cyMin);

    height = cyCur;

    return 1;
}


LRESULT CMainFrame::OnSetWindowHeight(WPARAM wParam, LPARAM /*lParam*/)
{
    ASSERT(wParam <= FormDefaults::QuestionTextHeightMax);
    m_wndCapiSplitter.SetRowInfo(0, wParam, 0);
    m_wndCapiSplitter.RecalcLayout();
    return 1;
}


LRESULT CMainFrame::OnPartialSaveFromApp(WPARAM wParam, LPARAM /*lParam*/)
{
    m_bPartialSaveFromApp = true;
    m_bPartialSaveClearSkipped = ( wParam != 0 );

    OnPartialSaveCase();

    LRESULT iRet = m_bPartialSaveFromApp ? 1 : 0; // false if the case was not saved

    m_bPartialSaveFromApp = false;
    m_bPartialSaveClearSkipped = false;

    return iRet;
}


int CMainFrame::GetIntEditOption()
{
    return intEdtDlg.m_iOption;
}

void CMainFrame::SetIntEditOption(int iOption)
{
    intEdtDlg.m_iOption = iOption;
}


void CMainFrame::BuildKeyArray()
{
    CWaitCursor wait;
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetDocument();
    CRunAplEntry* pRunApl = pRunDoc->GetRunApl();

    if( pRunApl == NULL || !pRunApl->HasAppLoaded() )
        return;

    m_caseSummaries.clear();
    m_duplicateCaseSummaries.clear();

    DataRepository* pInputRepo = pRunApl->GetInputRepository();
    m_eRepoType = pInputRepo->GetRepositoryType();

    try
    {
        std::unique_ptr<std::regex> caseFilterRegex;
        CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
        if (pDoc && pDoc->GetPifFile()) {
            const CString csFilterRegex = pDoc->GetPifFile()->GetCaseListingFilter();
            if (!csFilterRegex.IsEmpty()) {
                try {
                    caseFilterRegex = std::make_unique<std::regex>(UTF8Convert::WideToUTF8(csFilterRegex));
                }
                catch (const std::regex_error&) {
                    AfxMessageBox(FormatText(_T("CaseListingFilter is an invalid ECMAScript regular expression: %s"), (LPCTSTR)csFilterRegex));
                }
            }
        }

        auto add_case_summaries = [&](CaseIterationCaseStatus case_status, std::vector<CaseSummary>& case_summaries)
        {
            CaseSummary case_summary;
            auto case_summary_iterator = pInputRepo->CreateIterator(CaseIterationContent::CaseSummary, case_status,
                m_bCaseTreeSortedOrder ? CaseIterationMethod::KeyOrder : CaseIterationMethod::SequentialOrder,
                CaseIterationOrder::Ascending);

            while( case_summary_iterator->NextCaseSummary(case_summary) )
            {
                if( caseFilterRegex == nullptr || std::regex_search(UTF8Convert::WideToUTF8(case_summary.GetKey()), *caseFilterRegex) )
                    case_summaries.push_back(case_summary);
            }
        };

        add_case_summaries(CaseIterationCaseStatus::All, m_caseSummaries);

        if( DataRepositoryHelpers::TypeSupportsDuplicates(m_eRepoType) )
            add_case_summaries(CaseIterationCaseStatus::DuplicatesOnly, m_duplicateCaseSummaries);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        ErrorMessage::Display(exception);
    }

    UpdateCaseStats();
}

void CMainFrame::UpdateCaseStats()
{
    m_iNumCases = 0;
    m_iTotalPartial = 0;
    m_iNumPartialAdd = 0;
    m_iNumPartialModify = 0;
    m_iVerified = 0;
    m_iPartialVerify = 0;
    m_iDeleted = 0;

    for( const auto& case_summary : m_caseSummaries )
    {
        if( case_summary.GetDeleted() )
        {
            ++m_iDeleted;
            continue; // Deleted don't count in total
        }

        ++m_iNumCases;

        if( case_summary.GetVerified() )
            ++m_iVerified;

        if( case_summary.IsPartial() )
        {
            ++m_iTotalPartial;

            switch( case_summary.GetPartialSaveMode() )
            {
            case PartialSaveMode::Add:
                ++m_iNumPartialAdd;
                break;

            case PartialSaveMode::Modify:
                ++m_iNumPartialModify;
                break;

            case PartialSaveMode::Verify:
                ++m_iPartialVerify;
                break;
            }
        }
    }

    // rather than count exact duplicates, assume each duplicate is two of the same case
    m_iDuplicates = m_duplicateCaseSummaries.size() / 2;
}

CEntryrunView* CMainFrame::GetRunView() {
    ASSERT(m_wndCapiSplitter.GetPane(1, 0)->IsKindOf(RUNTIME_CLASS(CEntryrunView)));
    return (CEntryrunView*)m_wndCapiSplitter.GetPane(1, 0);
}


class CGPSDialog : public CDialog
{
    GPSThreadInfo * gpsTI;

public:

    CGPSDialog(UINT nIDTemplate,GPSThreadInfo * pGPSInfo)
        : CDialog(nIDTemplate)
    {
        gpsTI = pGPSInfo;
    }

    BOOL OnInitDialog()
    {
        GetDlgItem(IDC_GPS_DLG_TEXT)->SetWindowText(gpsTI->windowText);
        SetTimer(1,GPS_INTERVAL_DIALOG_CHECK,NULL);
        return true;
    }

    void OnTimer(UINT nIDEvent);

    void OnCancel()
    {
        gpsTI->cancelRead = true;
        EndDialog(IDCANCEL);
    }

protected:
    DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(CGPSDialog, CDialog)
    ON_WM_TIMER()
END_MESSAGE_MAP()


void CGPSDialog::OnTimer(UINT nIDEvent) // 20110525
{
    if( gpsTI->successfulRead || gpsTI->cancelRead )
    {
        KillTimer(nIDEvent);
        EndDialog(IDOK);
    }

    else if( gpsTI->numReadIntervals <= 0 ) // time out
    {
        KillTimer(nIDEvent);
        OnCancel();
    }

    else
        gpsTI->numReadIntervals--;
}


LRESULT CMainFrame::OnShowGPSDialog(WPARAM wParam, LPARAM lParam) // 20110524
{
    CDialog* pDlg = new CGPSDialog(IDD_DLG_GPS,(GPSThreadInfo *)lParam);
    return (LRESULT)pDlg;
}


LRESULT CMainFrame::OnShowProgressDialog(WPARAM wParam, LPARAM lParam)
{
    ASSERT(m_pProgressDlg == NULL);

    // Show the window if it isn't already visible
    if (!m_pProgressDlg) {
        m_pProgressDlg = new ProgressDialog();
        m_pProgressDlg->ShowModeless(*((CString*)lParam), this);
    }
    return 0;
}

LRESULT CMainFrame::OnHideProgressDialog(WPARAM wParam, LPARAM lParam)
{
    if (m_pProgressDlg) {
        m_pProgressDlg->DestroyWindow();
        delete m_pProgressDlg;
        m_pProgressDlg = NULL;
    }
    return 0;
}

LRESULT CMainFrame::OnUpdateProgressDialog(WPARAM wParam, LPARAM lParam)
{
    ASSERT(m_pProgressDlg != NULL);
    if (m_pProgressDlg) {
        return m_pProgressDlg->Update(wParam, lParam);
    }
    return 0;
}


void CMainFrame::DoInitialApplicationLayout(CNPifFile* pPifFile)
{
    Application* pApp = pPifFile->GetApplication();

    // set the proper field background colors
    GetRunView()->SetupFieldColors(pApp->GetRuntimeFormFiles().front().get());

    // go in full screen mode if necessary
    while( m_eScreenView != pPifFile->GetFullScreenFlag() )
        OnFullscreen();

    // make sure that the CAPI text window appears correctly
    m_wndCapiSplitter.m_bUseQuestionText = pApp->GetUseQuestionText();

    if( !m_wndCapiSplitter.m_bUseQuestionText )
        m_wndCapiSplitter.SetRowInfo(0,0,0);

    m_wndCapiSplitter.RecalcLayout();
    m_wndCapiSplitter.RedrawWindow();
}


LRESULT CMainFrame::OnChangeInputRepository(WPARAM wParam,LPARAM lParam)
{
    // the input repository was changed via a call to open/close/setfile so...

    // update the case listing
    BuildKeyArray();
    GetCaseView()->BuildTree();

    // and the title bar
    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetDocument();
    CString csTitle = pRunDoc->MakeTitle();
    pRunDoc->SetTitle(csTitle);
    SetWindowText(csTitle);

    // close the old operator statistics log and open a new one
    pRunDoc->OpenOperatorStatisticsLog();

    return 0;
}


LRESULT CMainFrame::OnWindowTitleQuery(WPARAM wParam, LPARAM lParam)
{
    bool get_title = (bool)wParam;
    CString& window_title = *((CString*)lParam);

    CEntryrunDoc* pRunDoc = (CEntryrunDoc*)GetDocument();

    if( get_title )
        window_title = pRunDoc->GetWindowTitle();

    else
    {
        pRunDoc->SetWindowTitle(window_title);
        SetWindowText(pRunDoc->MakeTitle());
    }

    return 0;
}


LRESULT CMainFrame::OnControlParadataKeyingInstance(WPARAM wParam, LPARAM lParam)
{
    // starting a new case
    if( wParam == 0 )
        m_keyingInstance = std::make_shared<Paradata::KeyingInstance>();

    // returning the object
    else
    {
        std::shared_ptr<Paradata::KeyingInstance>* ptr_keying_instance = (std::shared_ptr<Paradata::KeyingInstance>*)lParam;
        *ptr_keying_instance = m_keyingInstance;
        m_keyingInstance.reset();
    }

    return 0;
}

Paradata::KeyingInstance* CMainFrame::GetParadataKeyingInstance()
{
    return m_keyingInstance.get();
}


void CMainFrame::OnCSProSettings()
{
    SettingsDlg settings_dlg;
    settings_dlg.DoModal();
}


LRESULT CMainFrame::OnEngineUI(WPARAM wParam, LPARAM lParam)
{
    if( m_engineUIProcessor == nullptr )
    {
        CEntryrunDoc* pDoc = (CEntryrunDoc*)GetActiveDocument();
        m_engineUIProcessor = std::make_unique<EngineUIProcessor>(pDoc->GetPifFile(), true);
    }

    return m_engineUIProcessor->ProcessMessage(wParam, lParam);
}


LRESULT CMainFrame::OnRunOnUIThread(WPARAM wParam, LPARAM /*lParam*/)
{
    UIThreadRunner* ui_thread_runner = reinterpret_cast<UIThreadRunner*>(wParam);
    ui_thread_runner->Execute();
    return 1;
}


LRESULT CMainFrame::OnGetApplicationShutdownRunner(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return reinterpret_cast<LRESULT>(&m_applicationShutdownRunner);
}


LRESULT CMainFrame::OnActionInvokerEngineProgramControlExecuted(WPARAM wParam, LPARAM /*lParam*/)
{
    // code modified from OnUserbarUpdate
    CEntryrunDoc* pDoc = GetDocument();
    CRunAplEntry* pRunApl = pDoc->GetRunApl();

    if( pRunApl->IsStopRequested() )
    {
        pDoc->SetCurField(NULL); // this and the next line will keep OnStop from asking about a partial save
        pDoc->SetQModified(false);
        OnStop();
    }

    else
    {
        CEntryrunView* pView = GetRunView();

        pRunApl->SetProgressForPreEntrySkip();
        pView->PostMessage(UWM::CSEntry::MoveToField);
    }

    return 0;
}


void CMainFrame::OnViewQuestionnaire()
{
    CEntryrunDoc* pDoc = GetDocument();

    if( pDoc->GetAppMode() != NO_MODE )
    {
        pDoc->GetRunApl()->ViewCurrentCase();
    }

    else
    {
        CTreeCtrl& caseTree = GetCaseView()->GetTreeCtrl();
        const NODEINFO* pNodeInfo = reinterpret_cast<const NODEINFO*>(caseTree.GetItemData(caseTree.GetSelectedItem()));
        ASSERT(pNodeInfo != nullptr);

        pDoc->GetRunApl()->ViewCase(pNodeInfo->case_summary.GetPositionInRepository());
    }
}


void CMainFrame::OnUpdateViewQuestionnaire(CCmdUI* pCmdUI)
{
    const CEntryrunDoc* pDoc = GetDocument();
    bool enable;

    // if a case is open, enable this option
    if( pDoc->GetAppMode() != NO_MODE )
    {
        enable = true;
    }

    // otherwise enable it when a case is selected in the case tree and viewing cases is not locoked
    else
    {
        const CNPifFile* pPifFile = pDoc->GetPifFile();

        enable = ( pPifFile != nullptr && !pPifFile->GetViewLockFlag() &&
                   GetCaseView()->GetTreeCtrl().GetCount() > 0 &&
                   GetCaseView()->GetTreeCtrl().GetSelectedItem() != nullptr );
    }

    pCmdUI->Enable(enable);
}
