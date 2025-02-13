#pragma once

#include <zformf/ConditionGrid.h>


/////////////////////////////////////////////////////////////////////////////
// CQSFCndView form view

class CQSFCndView : public CView
{

// Members
public:
    CCondGrid   m_gridCond;
    HACCEL  m_hAccel;                   // RTF-specific accelerator keys (ctrl+b, ctrl+i, etc.)


private:
    int         m_iCount;


    LOGFONT     m_lf;

    int         m_iHeight;
    int         m_iLineHeight;




protected:
    CQSFCndView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CQSFCndView)

// Attributes
public:

// Operations
public:
    void ResizeGrid(void);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CQSFCndView)
    public:
    virtual void OnFinalRelease();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnInitialUpdate();
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CQSFCndView();

    CapiEditorViewModel& GetViewModel();
    CFormDoc* GetFormDoc();

    // Generated message map functions
protected:
    //{{AFX_MSG(CQSFCndView)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
    afx_msg void OnEditDelete();
    afx_msg void OnEditAdd();
    afx_msg void OnUpdateEditAdd(CCmdUI* pCmdUI);
    afx_msg void OnEditInsert();
    afx_msg void OnUpdateEditInsert(CCmdUI* pCmdUI);
    afx_msg void OnEditCopy();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnEditModify();
    afx_msg void OnUpdateEditModify(CCmdUI* pCmdUI);
    afx_msg void OnEditCut();
    afx_msg void OnEditPaste();
    afx_msg void OnEditUndo();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
