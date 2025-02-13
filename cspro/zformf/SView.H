#pragma once
//***************************************************************************
//  File name: MyView.h
//
//  Description:
//       My customized CHHGenericFormater object
//
//  History:    Date       Author   Comment
//              ---------------------------
//              08 Jun 98   gsf     Created for Measure 1.0
//
//***************************************************************************

#include <zformf/zFormF.h>
#include <zEdit2O/LogicView.h>

/////////////////////////////////////////////////////////////////////////////
// CFSourceEditView view

class CLASS_DECL_ZFORMF CFSourceEditView : public CLogicView {

    DECLARE_DYNCREATE(CFSourceEditView)

// Members
protected:

public:
    CFSourceEditView();
    ~CFSourceEditView();

// Attributes
public:

// Operations
public:
    CLogicCtrl* GetEditCtrl(void) { return CLogicView::GetLogicCtrl(); }
private:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFSourceEditView)
    public:
    virtual void OnFinalRelease();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //}}AFX_VIRTUAL

    // Generated message map functions
protected:
    //{{AFX_MSG(CFSourceEditView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnUpdateEditCutOrCopy(CCmdUI* pCmdUI);
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnFrmEditFind();
    afx_msg void OnUpdateFrmEditFind(CCmdUI* pCmdUI);
    afx_msg void OnFindNext();
    afx_msg void OnUpdateFindNext(CCmdUI* pCmdUI);
    afx_msg void OnReplace();
    afx_msg void OnUpdateReplace(CCmdUI* pCmdUI);
    afx_msg void OnEditUndo();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnViewForm();
    afx_msg void OnQsfEditor();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnShiftF10();
    afx_msg void OnUpdateLogicIsShowing(CCmdUI* pCmdUI);
    afx_msg void OnFormatLogic();
    afx_msg void OnPeekLogicWord();
    afx_msg void OnGotoLogicWord();
    afx_msg void OnCommentCode();
    afx_msg void OnDuplicateLine();

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};
