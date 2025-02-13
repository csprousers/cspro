#pragma once

//***************************************************************************
//  File name: OrdChWnd.h
//
//  Description:
//       Order child window definitions
//
//  History:    Date       Author   Comment
//              ---------------------------
//              04 Jun 99   gsf     Created for Measure 1.0
//
//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// COrderChildWnd frame

#include <zOrderF/zOrderF.h>
#include <zDesignerF/ApplicationChildWnd.h>
#include <zDesignerF/LogicDialogBar.h>
#include <zDesignerF/LogicReferenceWnd.h>
#include <zDesignerF/QuestionnaireView.h>

class COSourceEditView;


class CLASS_DECL_ZORDERF COrderChildWnd : public ApplicationChildWnd
{
    DECLARE_DYNCREATE(COrderChildWnd)

protected:
    COrderChildWnd();           // protected constructor used by dynamic creation

public:
    virtual ~COrderChildWnd();

    // ApplicationChildWnd overrides
    CLogicView* GetSourceLogicView() override;
    CLogicCtrl* GetSourceLogicCtrl() override;

    LogicDialogBar& GetLogicDialogBar() override       { return m_logicDlgBar; }
    LogicReferenceWnd& GetLogicReferenceWnd() override { return m_logicReferenceWnd; }

    // other methods
    COSourceEditView* GetOSourceView();

    const CString& GetApplicationName() const       { return m_sApplicationName; }
    void SetApplicationName(const CString& sString) { m_sApplicationName = sString; }

public:
    void ActivateFrame(int nCmdShow = -1) override;
    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CMDIFrameWnd* pParentWnd = NULL, CCreateContext* pContext = NULL) override;

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;

protected:
    DECLARE_MESSAGE_MAP()

    afx_msg void OnSysCommand( UINT nID, LPARAM lParam );
    afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
    afx_msg void OnRun();
    afx_msg void OnCompile();
    afx_msg void OnPeekLogicWord();
    afx_msg void OnGotoLogicWord();
    afx_msg void OnCommentCode();
    afx_msg void OnDuplicateLine();
    afx_msg void OnFormatLogic();
    afx_msg void OnViewQuestionnaire();
    afx_msg void OnUpdateViewQuestionnaire(CCmdUI* pCmdUI);
private:
    template<typename CF>
    void ModifyDocumentAndDoWithLogicCtrl(CF callback_function);

private:
    CToolBar            m_wndToolBarEdit;
    CString             m_sApplicationName;

    LogicDialogBar    m_logicDlgBar;
    LogicReferenceWnd m_logicReferenceWnd;
    BOOL              m_bQuestionnaireView;
    QuestionnaireView* m_pQuestionnaireView;
    COSourceEditView* m_pOSourceEditView;

public:
    QuestionnaireView* GetQuestionnaireView() { return m_pQuestionnaireView; }
    BOOL isQuestionnaireView() {return m_bQuestionnaireView; }
    afx_msg void OnViewLogic();
    afx_msg void OnUpdateViewLogic(CCmdUI* pCmdUI);
    afx_msg void OnUpdateOrdCompile(CCmdUI* pCmdUI);
    afx_msg void OnUpdateLogicFormat(CCmdUI* pCmdUI);
};
