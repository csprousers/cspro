#pragma once

//***************************************************************************
//  File name: DDChWnd.h
//
//  Description:
//       Data Dictionary child window definition
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Aug 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include <zDictF/zDictF.h>
#include <zDictF/DictDialogBar.h>
#include <zDictF/DSplWnd.h>
#include <zUToolO/OxFWndDk.h>
#include <zDesignerF/QuestionnaireView.h>

class CDictItem;


class CLASS_DECL_ZDICTF CDictChildWnd : public COXMDIChildWndSizeDock
{
    DECLARE_DYNCREATE(CDictChildWnd)

    friend class CDictDialogBar;

protected:
    CDictChildWnd();

public:
    TCHAR GetDecimal() const { return m_cDecimal; }

    CDictDialogBar* GetDictDlgBar() { return &m_dictDlgBar; }

    QuestionnaireView* GetQuestionnaireView() { return m_pQuestionnaireView; }
    BOOL isQuestionnaireView() const          { return m_bQuestionnaireView;  }

    void ActivateFrame(int nCmdShow = -1) override;
    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CMDIFrameWnd* pParentWnd = NULL, CCreateContext* pContext = NULL) override;

    void SetFindActive(bool bFind);
    LRESULT OnFind(WPARAM wParam, LPARAM lParam);

    void DisplayActiveMode();

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
    afx_msg void OnSysCommand( UINT nID, LPARAM lParam );
    afx_msg void OnViewLayout();
    afx_msg void OnUpdateEditFind(CCmdUI* pCmdUI);
    afx_msg void OnEditFind();
    afx_msg void OnFilePageSetup();
    afx_msg void OnUpdateViewLayout(CCmdUI* pCmdUI);
    afx_msg void OnViewAliases();
    afx_msg void OnDictionaryAnaylsis();
    afx_msg void OnViewProperties();
    afx_msg void OnUpdateViewProperties(CCmdUI* pCmdUI);
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnUpdateViewQuestionnaire(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditLanguages(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditRelation(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditSecurityOptions(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDictionaryMacros(CCmdUI* pCmdUI);
    afx_msg void OnUpdateOptionsEnableBinaryItems(CCmdUI* pCmdUI);
    afx_msg void OnUpdateOptionsAbsRel(CCmdUI* pCmdUI);
    afx_msg void OnUpdateOptionsZeroFill(CCmdUI* pCmdUI);
    afx_msg void OnUpdateOptionsDecChar(CCmdUI* pCmdUI);
    afx_msg void OnViewDictionary();
    afx_msg void OnUpdateViewDictionary(CCmdUI* pCmdUI);

public:
    int m_iGridViewPaneSize;

protected:
    CDictSplitterWnd m_wndSplitter;
    BOOL m_bLayout;
    BOOL m_bQuestionnaireView;
    BOOL m_bViewPropertiesPanel;
    TCHAR m_cDecimal;
    QuestionnaireView* m_pQuestionnaireView;

private:
    CDictDialogBar m_dictDlgBar;
};
