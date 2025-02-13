#pragma once
/////////////////////////////////////////////////////////////////////////////
//
// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include <CSEntry/CaseView.h>
#include <CSEntry/StatDlg.h>
#include <CSEntry/RunView.h>
#include <CSEntry/XSplWnd.h>

#include <zUtilO/CustomFont.h>
#include <zUtilO/BCMenu.h>
#include <zUtilF/ApplicationShutdownRunner.h>
#include <ZBRIDGEO/npff.h>

#include <zParadataO/KeyingInstance.h>
#include <zEngineF/EngineUI.h>

class CEntryrunView;
class CValueSetView;
class CLeftView;
class CCaseTree;
class ObjectTransporter;
class ProgressDialog;
class QSFView;

class CMainFrame : public CFrameWnd
{
public:
    BCMenu  m_menu;
    BOOL    m_bClose;

    int     m_iNumCases;
    int     m_iTotalPartial;
    int     m_iNumPartialAdd;
    int     m_iNumPartialModify;
    int     m_iVerified;
    int     m_iPartialVerify;
    int     m_iDeleted;
    int     m_iDuplicates;
    bool    m_bPage1StatusChangeStopFocusChangeHack;

protected: // create from serialization only
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

    CSplitterWnd m_wndSplitter;
    CXSplitterWnd   m_wndCapiSplitter;

    // Attributes
private:
    bool    m_bPause;

    FULLSCREEN m_eScreenView;

    bool    m_bShowCaseLabels;
    bool    m_bViewNames;
    bool    m_bPartialSave;
    int     m_iWidth;
    bool    m_bTreeEnable;

    CLeftView*      m_pLeftView;


    // Operations
public:
    friend class CEntryrunView;
    friend class CEntryrunApp;


        LONG IsUniqNames (WPARAM wParam, LPARAM lParam);

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void ActivateFrame(int nCmdShow = -1);
protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
        //virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

    // Implementation
public:
    void UpdateToolBar();
    QSFView* GetQTxtView();

    CEntryrunView* GetRunView();

    CLeftView* GetLeftView() { return m_pLeftView; }

    CCaseView* GetCaseView(); //FABN Nov 5, 2002
    CCaseTree* GetCaseTree();       //FABN Nov 6, 2002


    void Start();
    virtual ~CMainFrame();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;
    CReBar      m_wndReBar;
    //      CDEDlgBar      m_wndDlgBar;

    UserDefinedFonts m_userFonts; // 20100621

    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);

    LONG OnPartialSaveFromApp(UINT wParam, LONG lParam);  // RHF Oct 15, 2003

    LONG OnKeyChanged(WPARAM wParam, LPARAM lParam);
    LONG OnWriteCase(UINT wParam, LONG lParam);   // gsf 10-apr-00
    LONG OnEngineAbort(WPARAM wParam, LPARAM lParam);
    LONG OnPreprocessEngineMessage(WPARAM wParam, LPARAM lParam);
    LONG OnEngineMessage(WPARAM wParam, LPARAM lParam);   // RHF Jan 03, 2001

    LONG OnFieldBehavior(WPARAM wParam, LPARAM lParam); // RHF Nov 21, 2002
    LONG OnFieldVisibility(WPARAM wParam, LPARAM lParam);// RHF Nov 21, 2002

    LONG OnEngineRefresh(WPARAM wParam, LPARAM lParam);  // RHF Nov 19, 2001
    LONG OnEngineShowCapi(WPARAM wParam, LPARAM lParam); // RHF Nov 22, 2002
    LONG OnRefreshSelected(WPARAM wParam, LPARAM lParam);

    LONG OnSetSequential(WPARAM wParam, LPARAM lParam);
    LONG OnSetCapiText(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetWindowHeight(WPARAM wParam, LPARAM lParam);
    LRESULT OnSetWindowHeight(WPARAM wParam, LPARAM lParam);

    LONG OnUserbarUpdate(WPARAM wParam, LPARAM lParam);
    LONG OnSetMessageOverrides(WPARAM wParam, LPARAM lParam); // 20100518
    LONG OnUsingOperatorControlledMessages(WPARAM wParam, LPARAM lParam);
    LONG OnGetUserFonts(WPARAM wParam, LPARAM lParam); // 20100621
    LRESULT OnShowGPSDialog(WPARAM wParam, LPARAM lParam); // 20110524
    LRESULT OnEngineUI(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);
    LRESULT OnActionInvokerEngineProgramControlExecuted(WPARAM wParam, LPARAM lParam);

    bool SelectNextCaseForVerification();

    LONG OnShowProgressDialog(WPARAM wParam, LPARAM lParam);
    LONG OnHideProgressDialog(WPARAM wParam, LPARAM lParam);
    LONG OnUpdateProgressDialog(WPARAM wParam, LPARAM lParam);

    LRESULT OnControlParadataKeyingInstance(WPARAM wParam,LPARAM lParam);

    // Generated message map functions
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnUpdateStop(CCmdUI* pCmdUI);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void OnStats();
    afx_msg void OnUpdateStats(CCmdUI* pCmdUI);
    afx_msg void OnPartialSaveCase();
    afx_msg void OnUpdatePartialSaveCase(CCmdUI* pCmdUI);
    afx_msg void OnSynchronize();
    afx_msg void OnUpdateSynchronize(CCmdUI* pCmdUI);
    afx_msg void OnCSProSettings();
    afx_msg void OnClose();
    afx_msg void OnAddCase();
    afx_msg void OnModifyCase();
    afx_msg void OnVerifyCase();
    afx_msg void OnPause();
    afx_msg void OnUpdateAdd(CCmdUI* pCmdUI);
    afx_msg void OnUpdateModify(CCmdUI* pCmdUI);
    afx_msg void OnUpdateVerify(CCmdUI* pCmdUI);
    afx_msg void OnUpdatePause(CCmdUI* pCmdUI);
    afx_msg void OnPrevCase();
    afx_msg void OnUpdatePrevCase(CCmdUI* pCmdUI);
    afx_msg void OnNextCase();
    afx_msg void OnUpdateNextCase(CCmdUI* pCmdUI);
    afx_msg void OnFirstCase();
    afx_msg void OnUpdateFirstCase(CCmdUI* pCmdUI);
    afx_msg void OnLastCase();
    afx_msg void OnUpdateLastCase(CCmdUI* pCmdUI);
    afx_msg void OnNextGroupOcc();
    afx_msg void OnUpdateNextGroupOcc(CCmdUI* pCmdUI);
    afx_msg void OnNextGroup();
    afx_msg void OnUpdateNextGroup(CCmdUI* pCmdUI);
    afx_msg void OnNextLevel();
    afx_msg void OnUpdateNextLevel(CCmdUI* pCmdUI);
    afx_msg void OnNextLevelOcc();
    afx_msg void OnUpdateNextLevelOcc(CCmdUI* pCmdUI);
    afx_msg void OnPrevScreen();
    afx_msg void OnUpdatePrevScreen(CCmdUI* pCmdUI);
    afx_msg void OnNextScreen();
    afx_msg void OnUpdateNextScreen(CCmdUI* pCmdUI);
    afx_msg void OnDeletecase();
    afx_msg void OnUpdateDeletecase(CCmdUI* pCmdUI);
    afx_msg void OnInsertGroupocc();
    afx_msg void OnUpdateInsertGroupocc(CCmdUI* pCmdUI);
    afx_msg void OnDeleteGrpocc();
    afx_msg void OnUpdateDeleteGrpocc(CCmdUI* pCmdUI);
    afx_msg void OnSortgrpocc();
    afx_msg void OnUpdateSortgrpocc(CCmdUI* pCmdUI);
    afx_msg void OnInsertCase();
    afx_msg void OnUpdateInsertCase(CCmdUI* pCmdUI);
    afx_msg void OnInsertAfterOcc();
    afx_msg void OnUpdateInsertGrpoccAfter(CCmdUI* pCmdUI);
    afx_msg void OnFindcase();
    afx_msg void OnUpdateFindcase(CCmdUI* pCmdUI);
    afx_msg void OnPreviousPersistent();
    afx_msg void OnUpdatePreviousPersistent(CCmdUI* pCmdUI);
    afx_msg void OnGoto();
    afx_msg void OnUpdateGoto(CCmdUI* pCmdUI);
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    afx_msg void OnAdvtoend();
    afx_msg void OnUpdateAdvtoend(CCmdUI* pCmdUI);
    afx_msg void OnInsertNode();
    afx_msg void OnAddNode();
    afx_msg void OnUpdateAddInsertNode(CCmdUI* pCmdUI);
    afx_msg void OnDeleteNode();
    afx_msg void OnUpdateDeleteNode(CCmdUI* pCmdUI);
    afx_msg void OnUpdatePartialsInd (CCmdUI *pCmdUI);
    afx_msg void OnUpdateFieldInd(CCmdUI *pCmdUI);
    afx_msg void OnUpdateOccInd(CCmdUI *pCmdUI);
    afx_msg void OnUpdateModeInd(CCmdUI *pCmdUI);
    afx_msg void OnUpdateShowInd(CCmdUI *pCmdUI);

    afx_msg void OnFullscreen();
    afx_msg void OnUpdateFullscreen(CCmdUI* pCmdUI);
    afx_msg void OnShowCaseLabels();
    afx_msg void OnUpdateShowCaseLabels(CCmdUI* pCmdUI);
    afx_msg void OnToggleNames();
    afx_msg void OnUpdateToggleNames(CCmdUI* pCmdUI);
    afx_msg void OnSortorder();
    afx_msg void OnUpdateSortorder(CCmdUI* pCmdUI);
    afx_msg void OnInteractiveEditOptions();
    afx_msg void OnUpdateInteractiveEditOptions(CCmdUI* pCmdUI);
    afx_msg void OnInteractiveEdit();
    afx_msg void OnUpdateInteractiveEdit(CCmdUI* pCmdUI);
    afx_msg void OnLanguage();
    afx_msg void OnUpdateLanguage(CCmdUI* pCmdUI);
    afx_msg void OnCapiToggle();
    afx_msg void OnUpdateCapiToggle(CCmdUI* pCmdUI);
    afx_msg void OnCapiToggleAllVars();
    afx_msg void OnUpdateCapiToggleAllVars(CCmdUI* pCmdUI);

    afx_msg void OnViewRefusals();
    afx_msg void OnUpdateViewRefusals(CCmdUI* pCmdUI);
    afx_msg void OnViewCheat();
    afx_msg void OnUpdateViewCheat(CCmdUI* pCmdUI);
    afx_msg void OnToggleCaseTree();
    afx_msg void OnUpdateToggleCaseTree(CCmdUI* pCmdUI);
    afx_msg void OnViewAll();
    afx_msg void OnViewNotDeleted();
    afx_msg void OnViewDuplicate();
    afx_msg void OnViewPartial();
    afx_msg void OnUpdateViewAll(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewNotDeleted(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewDuplicate(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewPartial(CCmdUI* pCmdUI);
    afx_msg void OnUpdateNote(CCmdUI* pCmdUI);
    afx_msg void OnFieldNote();
    afx_msg void OnCaseNote();
    afx_msg void OnReviewNotes();
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnUpdateViewQuestionnaire(CCmdUI* pCmdUI);


    void OnStop(bool* close_csentry_after_stopping);
    afx_msg void OnStop() { OnStop(nullptr); }


//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    LRESULT OnGivenGoTo                     (WPARAM wParam, LPARAM lParam); //FABN Nov  4, 2002
    LRESULT OnRefreshCaseTree               (WPARAM wParam, LPARAM lParam); //FABN Nov  6, 2002
    LRESULT OnSelectiveRefreshCaseTree      (WPARAM wParam, LPARAM lParam); //FABN Nov 20, 2002

    LRESULT OnDeleteCaseTree                (WPARAM wParam, LPARAM lParam); //FABN Nov  7, 2002
    LRESULT OnCreateCaseTree                (WPARAM wParam, LPARAM lParam); // RHF Dec 19, 2002

    LRESULT OnGoToNode                      (WPARAM wParam, LPARAM lParam); //FABN Nov  8, 2002
    LRESULT OnPage1ChangeShowStatus         (WPARAM wParam, LPARAM lParam); //FABN Nov 11, 2002
    LRESULT OnRestoreEntryRunViewFocus      (WPARAM wParam, LPARAM lParam); //FABN Nov 11, 2002
    LRESULT OnTreeItemWithNothingToDo       (WPARAM wParam, LPARAM lParam); //FABN Nov 11, 2002
    LRESULT OnUnknownKey                    (WPARAM wParam, LPARAM lParam); //FABN Nov 20, 2002

    LRESULT OnAcceleratorKey                (WPARAM wParam, LPARAM lParam); //FABN Jan 15, 2003
    LRESULT OnRecalcLeftLayout              (WPARAM wParam, LPARAM lParam); //
    LRESULT OnCasesTreeFocus                (WPARAM wParam, LPARAM lParam); //FABN March 3, 2003

    LRESULT OnChangeInputRepository(WPARAM wParam,LPARAM lParam);
    LRESULT OnWindowTitleQuery(WPARAM wParam,LPARAM lParam);

    //ruben 10-apr-00
public:
    CEntryrunDoc*   GetDocument();

    void DoNote(bool bCaseNote);

    CArray<int,     int>        m_menuCmdIDArray;
    CArray<UINT,    UINT>       m_menuCmdCharAccelArray;
    CArray<CString, CString>    m_menuCmdFlagArray;
    CaseIterationCaseStatus     m_eCaseStatusToShow;
    DataRepositoryType  m_eRepoType;
    bool    m_bCaseTreeActiveOnStart;

    void SetShowNames(bool bFlag) {m_bViewNames = bFlag; }
    bool GetShowNames(void) {return m_bViewNames; }

    bool GetShowCaseLabels() const { return m_bShowCaseLabels; }

    int GetIntEditOption();
    void SetIntEditOption(int iOption);

    void DoInitialApplicationLayout(CNPifFile* pPifFile);

private:
    //FABN Jan 15, 2003
    void AddToAccelArrays( UINT uiMenuID, UINT nChar, bool bCtrl, bool bShift, bool bAlt );
    void GetCmd( UINT nChar, bool bCtrl, bool bShift,  bool bAlt, int* iCmdID, bool* bEnabled );

    bool CanStopAddMode();
    bool PreCaseLoadingStartActions(APP_MODE appMode);
    bool PostCaseLoadingStartActions(APP_MODE appMode,NODEINFO* pNodeInfo = nullptr,CRunAplEntry::ProcessModifyAction eModifyAction = CRunAplEntry::ProcessModifyAction::GotoNode);

    void ChangeViewCaseStatus(CaseIterationCaseStatus eStatus);

public:
    bool ModifyStarterHelper(NODEINFO* pNodeInfo,CRunAplEntry::ProcessModifyAction eModifyAction);

public:
    void SetSortFlag(bool bFlag) { m_bCaseTreeSortedOrder = bFlag; }
    bool GetSortFlag() { return m_bCaseTreeSortedOrder; }

    void BuildKeyArray();
    void UpdateCaseStats();
    const std::vector<CaseSummary>& GetCaseSummaries() const          { return m_caseSummaries; }
    const std::vector<CaseSummary>& GetDuplicateCaseSummaries() const { return m_duplicateCaseSummaries; }

    Paradata::KeyingInstance* GetParadataKeyingInstance();

private:
    bool    m_bRemoteEntry;    // Remote entry uses
    bool    m_bPartialSaveFromApp;
    bool    m_bPartialSaveClearSkipped;

    bool    m_bOnStop;

    ProgressDialog* m_pProgressDlg;

    std::vector<CaseSummary> m_caseSummaries;
    std::vector<CaseSummary> m_duplicateCaseSummaries;

    bool m_bCaseTreeSortedOrder;

    std::shared_ptr<Paradata::KeyingInstance> m_keyingInstance;

    std::unique_ptr<ObjectTransporter> m_objectTransporter;
    std::unique_ptr<EngineUIProcessor> m_engineUIProcessor;
    ApplicationShutdownRunner m_applicationShutdownRunner;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
