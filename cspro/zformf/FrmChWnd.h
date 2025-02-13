#pragma once

//***************************************************************************
//  File name: FrmChWnd.h
//
//  Description:
//       Forms child window definitions
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Feb 99   gsf     Created for Measure 1.0
//
//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// CFormChildWnd frame

#include <zformf/zFormF.h>
#include <zformf/SView.H>
#include <zformf/FSplWnd.h>
#include <zformf/QSFEView.h>
#include <zFormO/FormFile.h>
#include <zFormO/DragOptions.h>
#include <zDesignerF/ApplicationChildWnd.h>
#include <zDesignerF/LogicDialogBar.h>
#include <zDesignerF/LogicReferenceWnd.h>
#include <zDesignerF/QuestionnaireView.h>

class BoxToolbar;
class CDEDragOptions;
class CFormScrollView;

enum eViewMode { FormViewMode, LogicViewMode, QSFEditorViewMode, QuestionnaireViewMode };


class CLASS_DECL_ZFORMF CFormChildWnd : public ApplicationChildWnd
{
    DECLARE_DYNCREATE(CFormChildWnd)

    friend CFSourceEditView; //For BCMENU stuff to work
    friend CFormScrollView;
    friend CQSFEView;

protected:
    CFormChildWnd(); // protected constructor used by dynamic creation

public:
    virtual ~CFormChildWnd();

    // ApplicationChildWnd overrides
    CLogicView* GetSourceLogicView() override;
    CLogicCtrl* GetSourceLogicCtrl() override;

    LogicDialogBar& GetLogicDialogBar() override       { return m_logicDlgBar; }
    LogicReferenceWnd& GetLogicReferenceWnd() override { return m_logicReferenceWnd; }

    // other methods
    CFSourceEditView* GetSourceView();

    const DragOptions& GetDragOptions() const { return m_dragOptions; }
    DragOptions& GetDragOptions()             { return m_dragOptions; }

    bool ShowDragOptionsDlg();

    void ShowBoxToolBar();
    bool IsBoxToolBarShowing() const { return ( m_pBoxToolBar != nullptr ); }
    static void SetBoxToolBarLocation(const CPoint& point) { m_cBoxLocation = point; }

    BoxType GetCurBoxBtn() const { return m_eBoxBtnSel.value_or(BoxType::Etched); }

    bool    CanUserDrawBox()       { return ( m_bDrawBox && m_eBoxBtnSel.has_value() ); }
    void    CanUserDrawBox(bool b) { m_bDrawBox = b; }

    static CToolBar* CreateFormToolBar(CWnd* pParentWnd);

    bool    IsLogicViewActive();
    bool    IsFormViewActive ();
    bool    IsFormFrameActive();

    const CString& GetApplicationName()             { return m_sApplicationName; }
    void SetApplicationName(const CString& sString) { m_sApplicationName = sString; }

    void DisplayNoQuestionTextMode(bool bFormView = true);
    void DisplayQuestionTextMode(bool bFormView = true);
    void DisplayEditorMode();
    void DisplayQuestionnaireViewMode();
    bool GetUseQuestionText() const { return m_bUseQuestionText; }
    void DisplayActiveMode();
    void DisplayMultiLangMode();
    void DisplaySingleLangMode();

    void ShowCapiLanguage(wstring_view language_name);

    eViewMode GetViewMode() { return m_eViewMode;}
    void SaveHeightSettings();
    void GetHeightSettings();
    CQSFEView* GetQSFView1() {return m_pQSFEditView1;}
    CQSFEView* GetQSFView2() {return m_pQSFEditView2;}
    QuestionnaireView* GetQuestionnaireView() { return m_pQuestionnaireView; }

    void RunMultipleFieldPropertiesDialog(std::vector<CDEField*>* selected_fields, CDEGroup* pCurGroup, CString form_name);
    void SaveServerUrlToRegistry(const CString& server_url) const;
    CString GetServerUrlFromRegistry() const;

// Overrides
public:
    void ActivateFrame(int nCmdShow = -1) override;
protected:
    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CMDIFrameWnd* pParentWnd = NULL, CCreateContext* pContext = NULL) override;
    BOOL DestroyWindow() override;

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

// Implementation
protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSysCommand( UINT nID, LPARAM lParam );
    afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
    afx_msg void OnViewLogic();
    afx_msg void OnViewForm();
    afx_msg void OnSelectItems();
    afx_msg void OnUpdateSelectItems(CCmdUI* pCmdUI);
    afx_msg void OnBoxToolbar(UINT nID);
    afx_msg void OnUpdateBoxToolbar(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewForm(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewLogic(CCmdUI* pCmdUI);
    afx_msg void OnUpdateGenerateForm(CCmdUI* pCmdUI);
    afx_msg void OnGo();
    afx_msg void OnEditOptions();
    afx_msg void OnOptionsDataSources();
    afx_msg void OnUpdateIfApplicationIsAvailable(CCmdUI* pCmdUI);
    afx_msg void OnOptionsSynchronization();
    afx_msg void OnOptionsMapping();
    afx_msg void OnCompile();
    afx_msg void OnUpdateCompile(CCmdUI* pCmdUI);
    afx_msg void OnEditDragOptions();
    afx_msg void OnOptionsFieldProperties();
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnUpdateQsfEditor(CCmdUI* pCmdUI);
    afx_msg void OnClose();
    afx_msg void OnAddcapiLang();
    afx_msg void OnUpdateIfUsingQuestionText(CCmdUI* pCmdUI);
    afx_msg void OnCapiMacros();
    afx_msg void OnRunasBch();
    afx_msg void OnUpdateRunasBch(CCmdUI* pCmdUI);
    afx_msg void OnGenerateBinary();
    afx_msg void OnPublishAndDeploy();
    afx_msg void OnEditStyles();

    LRESULT OnSwitchView(WPARAM wParam, LPARAM lParam);

public:
    afx_msg void OnQsfEditor();

private:
    bool SetupBoxToolBar();
    void DeleteBoxToolBar();

    CFormDoc* PreRunPublish();


// Attributes
public:
    CFormScrollView*        m_pFormView;

    int                     m_iQuestPaneSz; // for quest size in form view in capimode
    int                     m_iQsfVSz1; // for quest size  qsfview1
    int                     m_iQsfVSz2; // for quest size  qsfview2
    bool                    m_bMultiLangMode;
    bool                    m_bHideSecondLang; //Hide second language window

private:
    static std::unique_ptr<BoxToolbar> m_pBoxToolBar; // a ptr to the floating pick-a-box2draw toolbar :)
    static bool m_bDrawBox;                           // am i drawing boxes or doing trackers?
    static std::optional<BoxType> m_eBoxBtnSel;
    static std::optional<CPoint> m_cBoxLocation;

    CFSourceEditView*       m_pSourceEditView;

    LogicDialogBar          m_logicDlgBar;
    LogicReferenceWnd       m_logicReferenceWnd;

    CQSFEView*              m_pQSFEditView1;
    CQSFEView*              m_pQSFEditView2;
    QuestionnaireView*      m_pQuestionnaireView;
    DragOptions             m_dragOptions;
    CString                 m_sApplicationName;
    CFSplitterWnd           m_wndFSplitter;
    bool                    m_bUseQuestionText;

    eViewMode               m_eViewMode;
    bool                    m_bAppAssociated;
    bool                    m_bFirstTime;
public:
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnUpdateViewQuestionnaire(CCmdUI* pCmdUI);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
