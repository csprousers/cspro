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

    void OnSysCommand( UINT nID, LPARAM lParam );
    void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
    void OnRun();
    void OnCompile();
    void OnPeekLogicWord();
    void OnGotoLogicWord();
    void OnCommentCode();
    void OnFormatLogic();
    void OnUpdateFormatLogic(CCmdUI* pCmdUI);
    void OnViewQuestionnaire();
    void OnUpdateViewQuestionnaire(CCmdUI* pCmdUI);

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
    BOOL isQuestionnaireView() const          { return m_bQuestionnaireView; }

    void OnViewLogic();
    void OnUpdateViewLogic(CCmdUI* pCmdUI);
    void OnUpdateOrdCompile(CCmdUI* pCmdUI);
    void OnUpdateLogicFormat(CCmdUI* pCmdUI);
};
