#pragma once
//***************************************************************************
//
//  Description:
//
//  History:    Date       Author   Comment
//              ---------------------------
//
//***************************************************************************

#include <zEdit2O/LogicView.h>

/////////////////////////////////////////////////////////////////////////////
// COSourceEditView view

class CLASS_DECL_ZORDERF COSourceEditView : public CLogicView {

    DECLARE_DYNCREATE(COSourceEditView)

// Members
protected:

public:
    COSourceEditView();
    ~COSourceEditView();

// Attributes
public:

// Operations
public:
    CLogicCtrl* GetEditCtrl(void) { return CLogicView::GetLogicCtrl(); }
private:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COSourceEditView)
    public:
    virtual void OnFinalRelease();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //}}AFX_VIRTUAL
// Implementation
protected:
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(COSourceEditView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnUpdateEditCutOrCopy(CCmdUI* pCmdUI);
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnOrdEditFind();
    afx_msg void OnUpdateOrdEditFind(CCmdUI* pCmdUI);
    afx_msg void OnFindNext();
    afx_msg void OnUpdateFindNext(CCmdUI* pCmdUI);
    afx_msg void OnReplace();
    afx_msg void OnUpdateReplace(CCmdUI* pCmdUI);
    afx_msg void OnEditUndo();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
    afx_msg void OnShiftF10();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
