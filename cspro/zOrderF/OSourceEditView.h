#pragma once

//***************************************************************************
//
//  Description:
//
//  History:    Date       Author   Comment
//              ---------------------------
//
//***************************************************************************

#include <zEditO/LogicView.h>

/////////////////////////////////////////////////////////////////////////////
// COSourceEditView view

class CLASS_DECL_ZORDERF COSourceEditView : public LogicView {

    DECLARE_DYNCREATE(COSourceEditView)

// Members
protected:

public:
    COSourceEditView();
    ~COSourceEditView();

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

    afx_msg void OnSize(UINT nType, int cx, int cy);

    afx_msg void OnShiftF10();

public:
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
