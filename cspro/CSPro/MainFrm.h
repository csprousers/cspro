#pragma once

#include <CSPro/MDlgBar.h>
#include <zUtilO/BCMenu.h>
#include <zUtilF/ApplicationShutdownRunner.h>
#include <zUToolO/zUtoolO.h>
#include <zExTab/zExTab.h>
#include <zInterfaceF/LangDlgBar.h>
#include <ZBRIDGEO/npff.h>
#include <zTableF/TabDoc.h>

class ObjectTransporter;


class CMainFrame : public COXMDIFrameWndSizeDock
{
    DECLARE_DYNAMIC(CMainFrame)

public:
    CMainFrame();

    HMENU DictMenu();
    HMENU TableMenu();
    HMENU FormMenu();
    HMENU OrderMenu();
    HMENU DefaultMenu();

// Attributes
public:
    BCMenu  m_DictMenu;
    BCMenu  m_TableMenu;
    BCMenu  m_FormMenu;
    BCMenu  m_OrderMenu;
    BCMenu  m_DefaultMenu;

private:
    LPCTSTR m_pszClassName;
    CString m_csWindowText;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
public:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMainFrame();

public:
    PROCESS  m_eProcess;

protected:  // control bar embedded members
    CStatusBar  m_wndStatusBar;
    CReBar      m_wndReBar;
    CDialogBar  m_wndDlgBar;
    LangDlgBar  m_wndLangDlgBar;

    CToolBar*   m_pWndToolBar;
    CImageList* m_pWndToolBarImages;
    CToolBar*   m_pWndTabTBar;
    CToolBar*   m_pWndDictTBar;
    CToolBar*   m_pWndOrderTBar;
    CImageList* m_pWndOrderTBarImages;
    CToolBar*   m_pWndFormTBar;

    CComboBox   m_tabAreaComboBox;
    CComboBox   m_tabZoomComboBox;
    CButton     m_printViewCloseButton;

    BOOL        m_bDictToolbar;
    BOOL        m_bTabToolbar;
    BOOL        m_bFormToolbar;
    BOOL        m_bOrderToolbar;

    bool        m_bRemovingPossibleDuplicateProcs; // 20120613

private:
    CMDlgBar    m_SizeDlgBar;

    std::unique_ptr<ObjectTransporter> m_objectTransporter;
    ApplicationShutdownRunner m_applicationShutdownRunner;


public:
    CMDlgBar& GetDlgBar() { return m_SizeDlgBar; }

    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);

    LRESULT ShowToolBar(WPARAM wParam, LPARAM lParam);
    LRESULT HideToolBar(WPARAM wParam, LPARAM lParam);

    LRESULT OnFormUpdateStatusBar(WPARAM wParam, LPARAM lParam);

    LRESULT OnLaunchActiveApp(WPARAM wParam, LPARAM lParam);
    LRESULT OnLaunchActiveAppAsBch(WPARAM wParam, LPARAM lParam);
    LRESULT OnGenerateBinary(WPARAM wParam, LPARAM lParam); // 20100819: wParam will mean type now, 0 = binary, 1 = XML
    LRESULT OnPublishAndDeploy(WPARAM wParam, LPARAM lParam);

    BOOL IsOKToClose();

    LRESULT OnRunBatch(WPARAM wParam, LPARAM lParam);

    LRESULT SelectTab(WPARAM wParam, LPARAM lParam);

    LRESULT OnShowCapiText(WPARAM wParam, LPARAM lParam);

    LRESULT GetLangInfo(WPARAM wParam, LPARAM lParam);
    LRESULT ProcessLangs(WPARAM wParam, LPARAM lParam);

    // gets the application using this form file (or order); if there are multiple ones,
    // the user must select which one to use
    CAplDoc* GetApplicationUsingFormFile(wstring_view form_filename, bool silent = false);

    CAplDoc* ProcessFOForSrcCode(CDocument& document);

protected:
    LRESULT OnGetMessageTextSource(WPARAM wParam, LPARAM lParam);

private:
    template<typename T>
    T* GetNodeIdForSourceCode(T* pNodeId = nullptr);

public:
    void SetSourceCode(CAplDoc* pDoc);
    bool PutSourceCode(CFormID* pFormID, bool bForceCompile);
    bool CompileAll(CFormDoc* form_doc);

    void SetOSourceCode(CAplDoc* pAplDoc);
    bool PutOSourceCode(AppTreeNode* app_tree_node, bool bForceCompile);

    LRESULT ShowSrcCode(WPARAM wParam, LPARAM lParam);
    LRESULT UpdateSrcCode(WPARAM wParam, LPARAM lParam);
    LRESULT OnSelChange(WPARAM wParam, LPARAM lParam);
    LRESULT UpdateTabSrcCode(WPARAM wParam, LPARAM lParam);
    LRESULT CheckSyntax4TableLogic(WPARAM wParam, LPARAM lParam);

    LRESULT ShowOSrcCode(WPARAM wParam, LPARAM lParam);
    LRESULT UpdateOSrcCode(WPARAM wParam, LPARAM lParam);
    LRESULT IsNameUnique(WPARAM wParam, LPARAM lParam);

    LRESULT SetStatusBarPane(WPARAM wParam, LPARAM lParam);
    LRESULT OnIMSASetFocus(WPARAM wParam, LPARAM lParam);
    LRESULT OnIsCode(WPARAM wParam, LPARAM lParam);
    LRESULT OnFIsCode(WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateSymbolTblFlag(WPARAM wParam, LPARAM lParam);

    LRESULT OnGetApplication(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetFormFileOrDictionary(WPARAM wParam, LPARAM lParam);

    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);

    LRESULT OnCreateUniqueName(WPARAM wParam, LPARAM lParam);

    LRESULT OnGetDictionaryType(WPARAM wParam, LPARAM lParam);
    LRESULT OnDictNameChange(WPARAM wParam, LPARAM lParam);//On changing the names of dict/level/recordin the dictionary
    LRESULT OnDictValueLabelChange(WPARAM wParam, LPARAM lParam);

    bool CopyEnt2Bch(CAplDoc* pAplDoc);
    bool PreparePieceRun(CNPifFile* pPIFFile,PROCESS eProcess);

    LONG OnIMSATabConvert(WPARAM wParam, LPARAM lParam);

    bool PutTabSourceCode(const TableElementTreeNode& table_element_tree_node, bool bForceCompile);
    bool CheckSyntax4TableLogic(const TableElementTreeNode& table_element_tree_node, XTABSTMENT_TYPE eXTabStatementType);

    LRESULT IsTabNameUnique(WPARAM wParam, LPARAM lParam);
    LONG PutTallyProc (UINT, LPARAM);
    LONG ReplaceLvlProc4Area(UINT wParam ,LPARAM lParam);
    LONG RenameProc(UINT wParam, LPARAM lParam);
    LONG DeleteTblLogic (UINT, LPARAM);
    LRESULT ShowTblSrcCode(WPARAM wParam, LPARAM lParam);
    LRESULT ReconcileLinkObj(WPARAM wParam, LPARAM lParam);
    void SetTblSourceCode(CAplDoc* pDoc);
    bool DoEmulateBCHApp(Application* pApp) ; // called on a tab app
    bool UndoEmulateBCHApp(Application* pApp) ; // called on a tab app
    bool GetLinkTables(CTabulateDoc* pTabDoc, CArray<CLinkTable*,CLinkTable*>& aLinkTables , bool bSilent =true);

private:
    std::tuple<CAplDoc*, CDEItemBase*> GetCapiItemDetails(CFormDoc* pFormDoc, CFormID* form_id = nullptr);

public:
    LRESULT OnIsQuestion(WPARAM wParam, LPARAM lParam);
    LRESULT OnReconcileQsfFieldName(WPARAM wParam, LPARAM lParam); // 20120710
    LRESULT OnReconcileQsfDictName(WPARAM wParam, LPARAM lParam);

public:
    LRESULT OnGetApplicationPff(WPARAM wParam, LPARAM lParam);

    bool ForceLogicUpdate4Tab(CTabulateDoc* pTabDoc);

    void DoPostRunTabCleanUp(CNPifFile* pPifFile);

    // Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()

    void OnUpdateFrameTitle(BOOL bAddToTitle) override;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    afx_msg void OnClose();
    afx_msg void OnUpdateKeyOvr(CCmdUI* pCmdUI);
    afx_msg void OnEndSession(BOOL bEnding);
    afx_msg void OnDictType();
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    afx_msg void OnAbout1();
    LRESULT OnRunTab(WPARAM wParam, LPARAM lParam);
    afx_msg void OnUpdateAreaComboBox(CCmdUI* pCmdUI);
    afx_msg void OnUpdateZoomComboBox(CCmdUI* pCmdUI);
    afx_msg void OnDropFiles(HDROP hDropInfo); // 20110128
    afx_msg void OnUpdateIfApplicationIsAvailable(CCmdUI* pCmdUI);
    afx_msg void OnOptionsProperties();
    LRESULT OnSetExternalApplicationProperties(WPARAM wParam, LPARAM lParam);
    LRESULT IsReservedWord(WPARAM wParam, LPARAM lParam);
    LRESULT OnCapiMacros(WPARAM wParam, LPARAM lParam);

    LRESULT OnUpdateLanguageList(WPARAM wParam, LPARAM lParam);
    LRESULT OnSelectLanguage(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetCurrentLanguageName(WPARAM wParam, LPARAM lParam);
    afx_msg void OnChangeDictionaryLanguage();


    // document functions
    // --------------------------------------------------------------------------
private:
    CDocTemplate* GetDocTemplate(wstring_view extension);

    // the callback function to the foreach iterators should return true to keep processing
    template<typename DocumentType, typename CF>
    void ForeachDocument(CF callback_function);

    template<typename DocumentType, typename CF>
    void ForeachDocumentUsingDictionary(const CDataDict& dictionary, CF callback_function);

    template<typename CF>
    void ForeachApplicationDocumentUsingFormFile(const CDEFormFile& form_file, CF callback_function);

    template<typename CF>
    void ForeachLogicAndReportTextSource(CF callback_function);


    // logic functions
    // --------------------------------------------------------------------------
private:
    void OnViewLogic(int position_in_buffer);
    void GotoExternalLogicOrReportNode(const CDocument* pDoc, CLogicCtrl* logic_control, NullTerminatedString filename,
                                       bool using_line_number, int line_number_or_position_in_buffer);

protected:
    afx_msg void OnViewTopLogic();
    LRESULT OnLogicReference(WPARAM wParam, LPARAM lParam);
    LRESULT OnSymbolsAdded(WPARAM wParam, LPARAM lParam);
    LRESULT OnLogicAutoComplete(WPARAM wParam, LPARAM lParam);
    LRESULT OnLogicInsertProcName(WPARAM wParam, LPARAM lParam);

    LRESULT OnCanAddResourceFolder(WPARAM wParam, LPARAM lParam);
    LRESULT OnCreateResourceFolder(WPARAM wParam, LPARAM lParam);

    LRESULT OnUpdateApplicationExternalities(WPARAM wParam, LPARAM lParam);
    LRESULT OnFindOpenTextSourceEditable(WPARAM wParam, LPARAM lParam);

    LRESULT OnGetCompilerHelperCache(WPARAM wParam, LPARAM lParam);

    LRESULT OnGoToLogicError(WPARAM wParam, LPARAM lParam);

    LRESULT OnGetLexerLanguage(WPARAM wParam, LPARAM lParam);


    // report functions
    // --------------------------------------------------------------------------
protected:
    LRESULT OnEditReportProperties(WPARAM wParam, LPARAM lParam);

    void OnViewReportPreview();
    void OnUpdateViewReportPreview(CCmdUI* pCmdUI);

private:
    const TextSource* GetHtmlReportTextSourceCurrentlyEditing(std::wstring* report_name_for_report_preview);


    // Code menu handlers
    // --------------------------------------------------------------------------
protected:
    void OnUpdateIfLogicIsShowing(CCmdUI* pCmdUI);

    void OnPasteStringLiteral();
    void OnUpdatePasteStringLiteral(CCmdUI* pCmdUI);

    void OnStringEncoder();
    void OnPathAdjuster();

    void OnSymbolAnalysis();

    void OnDeprecationWarnings(UINT nID);
    void OnUpdateDeprecationWarnings(CCmdUI* pCmdUI);

    void OnCodeFoldingLevel(UINT nID);
    void OnUpdateCodeFoldingLevel(CCmdUI* pCmdUI);
    void OnCodeFoldingAction(UINT nID);
    void OnUpdateCodeFoldingAction(CCmdUI* pCmdUI);


    // other functions
    // --------------------------------------------------------------------------
    LRESULT OnGetDesignerIcon(WPARAM wParam, LPARAM lParam);
    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);
    LRESULT OnEngineUI(WPARAM wParam, LPARAM lParam);

    // property grid functions
    LRESULT OnRedrawPropertyGrid(WPARAM wParam, LPARAM lParam);


public:
    static FrameType GetFrameType(CWnd* pWnd = nullptr);
};
