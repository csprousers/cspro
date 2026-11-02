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
    COrderChildWnd(); // protected constructor used by dynamic creation

public:
    // ApplicationChildWnd overrides
    LogicView* GetSourceLogicView() override;
    LogicCtrl* GetSourceLogicCtrl() override;

    LogicDialogBar& GetLogicDialogBar() override       { return m_logicDlgBar; }
    LogicReferenceWnd& GetLogicReferenceWnd() override { return m_logicReferenceWnd; }

    // other methods
    QuestionnaireView* GetQuestionnaireView() { return m_questionnaireView; }

    const CString& GetApplicationName() const    { return m_applicationName; }
    void SetApplicationName(const CString& name) { m_applicationName = name; }

public:
    void ActivateFrame(int nCmdShow = -1) override;
    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW, const RECT& rect = rectDefault, CMDIFrameWnd* pParentWnd = NULL, CCreateContext* pContext = NULL) override;

protected:
    DECLARE_MESSAGE_MAP()

    void OnSysCommand( UINT nID, LPARAM lParam );
    void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);

    void OnRun();
    void OnCompile();
    void OnUpdateRunCompile(CCmdUI* pCmdUI);

    void OnViewQuestionnaire();
    void OnUpdateViewQuestionnaire(CCmdUI* pCmdUI);

    void OnViewLogic();
    void OnUpdateViewLogic(CCmdUI* pCmdUI);

private:
    COSourceEditView* GetOSourceView();

private:
    CString m_applicationName;
    LogicDialogBar m_logicDlgBar;
    LogicReferenceWnd m_logicReferenceWnd;
    bool m_viewingQuestionnaire;
    QuestionnaireView* m_questionnaireView;
    COSourceEditView* m_sourceEditView;
};
