#pragma once

#include <zTableF/zTableF.h>
#include <zDesignerF/ApplicationChildWnd.h>
#include <zDesignerF/LogicDialogBar.h>
#include <zDesignerF/LogicReferenceWnd.h>
#include <zTableF/TSView.H>
#include <zTableF/PrtView.h>

class CCompFmtDlg;
class CTreePropertiesDlg;
class CTabView;


class CLASS_DECL_ZTABLEF CTableChildWnd : public ApplicationChildWnd
{
    DECLARE_DYNCREATE(CTableChildWnd)

protected:
    CTableChildWnd();           // protected constructor used by dynamic creation


// Operations
public:
    virtual ~CTableChildWnd();

    // ApplicationChildWnd overrides
    CLogicView* GetSourceLogicView() override;
    CLogicCtrl* GetSourceLogicCtrl() override;

    LogicDialogBar& GetLogicDialogBar() override       { return m_logicDlgBar; }
    LogicReferenceWnd& GetLogicReferenceWnd() override { return m_logicReferenceWnd; }

    // other methods
    bool IsViewer() const          { return m_bIsViewer; }
    void SetIsViewer(bool bViewer) { m_bIsViewer = bViewer; }

    CTSourceEditView* GetSourceView();


    CTabPrtView*    GetPrintView(void);
    CTabView*       GetTabView(void);

    void PrintFromNonPrintView(int currTbl);
    bool CreatePrintView();
    void DestroyPrintView();
    void DynamicMenu4Logic(bool bDelete =true);

public:
    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CMDIFrameWnd* pParentWnd = NULL, CCreateContext* pContext = NULL) override;
    void ActivateFrame(int nCmdShow = -1) override;

// Implementation
public:
    void OnViewTable();
    bool IsTableFrameActive (void);
    bool IsLogicViewActive (void);
    virtual bool IsTabViewActive();
    void ShowTabView(bool bDesignView = true);
    LRESULT OnTabSetIcon(WPARAM wParam,LPARAM lParam);
    bool IsDesignView() const{ return m_bDesignView;}
    void SetDesignView(bool bDesignView) {m_bDesignView = bDesignView;}
    void OnParms(bool bSubTable=false);

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
    afx_msg void OnViewLogic();
    afx_msg void OnUpdateViewLogic(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewTable(CCmdUI* pCmdUI);
    afx_msg void OnTblCompile();
    afx_msg void OnUpdateTblCompile(CCmdUI* pCmdUI);
    afx_msg void OnEditTableTallyAttributes();
    afx_msg void OnUpdateEditTableTallyAttributes(CCmdUI* pCmdUI);
    afx_msg void OnViewPageprintview();
    afx_msg void OnUpdateViewPageprintview(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewFacing(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewBooklayout(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewZoom(CCmdUI* pCmdUI);
    afx_msg void OnRuntab();
    afx_msg void OnUpdateRuntab(CCmdUI* pCmdUI);
    //}}AFX_MSG

    afx_msg void OnViewPreviousTable();
    afx_msg void OnUpdateViewPreviousTable(CCmdUI *pCmdUI);
    afx_msg void OnViewNextTable();
    afx_msg void OnUpdateViewNextTable(CCmdUI *pCmdUI);
    afx_msg void OnEditSpecifyunit();
    afx_msg void OnUpdateEditSpecifyunit(CCmdUI *pCmdUI);
    afx_msg void OnEditOptions();
    afx_msg void OnUpdateEditOptions(CCmdUI *pCmdUI);
    afx_msg void OnFilePrintSetup();
    afx_msg void OnUpdateFilePrintSetup(CCmdUI *pCmdUI);
    afx_msg void OnDesignView();
    afx_msg void OnUpdateDesignView(CCmdUI *pCmdUI);
    afx_msg void OnExportTFT();
    afx_msg void OnImportTFT();
    afx_msg void OnEditGeneratelogic();
    afx_msg void OnUpdateEditGeneratelogic(CCmdUI *pCmdUI);
    afx_msg void OnEditExcludeTbl();
    afx_msg void OnUpdateExcludeTbl(CCmdUI *pCmdUI);
    afx_msg void OnEditExcludeAllButThis();
    afx_msg void OnUpdateEditExcludeAllButThis(CCmdUI *pCmdUI);
    afx_msg void OnEditIncludeAll();
    afx_msg void OnUpdateEditIncludeAll(CCmdUI *pCmdUI);

    // Attributes
public:
    // navigation bar support
    CPrtViewNavigationBar   m_wndNavDlgBar;       // navigation dialog bar (owns a toolbar with buttons for first/last/next/prev page navigation)

private:
    bool                    m_bIsViewer;
    CTSourceEditView*       m_pSourceEditView;

    LogicDialogBar    m_logicDlgBar;
    LogicReferenceWnd m_logicReferenceWnd;

    CTabPrtView*            m_pPrintView;
    CView*                  m_pOldView;
    bool                    m_bDesignView;
};
