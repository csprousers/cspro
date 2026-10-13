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

#include <zTableF/zTableF.h>
#include <zEditO/LogicView.h>

/////////////////////////////////////////////////////////////////////////////
// CTSourceEditView view


class CLASS_DECL_ZTABLEF CTSourceEditView : public LogicView {

    DECLARE_DYNCREATE(CTSourceEditView)

    // Members
protected:

    LogicCtrl* m_pWndEditCtrl;

public:
    CTSourceEditView();
    ~CTSourceEditView();

    // Overrides
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
    DECLARE_MESSAGE_MAP()

    //{{AFX_MSG(CTSourceEditView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnViewTable();
    //}}AFX_MSG
public:
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
