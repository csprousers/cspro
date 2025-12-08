#pragma once
//***************************************************************************
//  File name: TVView.h
//
//  Description:
//       Interface for the CTVView view class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//              4  Jan 03   csc       Add double-click support (copy to clipboard)
//              4  Jan 03   csc       Add support for changing font size
//             16  Jan 05   csc       Mouse wheel panning support added
//
//***************************************************************************

#include <zUtilO/PageSetup.h>

class CFindDlg;

class CTVView : public CBlockScrollView
{
protected: // create from serialization only
        CTVView();
        DECLARE_DYNCREATE(CTVView)

// Attributes
public:
        CTVDoc* GetDocument();

private:
    CRulerViewMgr       m_rulerMgr;           // manages the different ruler views
    BOOL                m_bRulersActivated,   // TRUE if rulers are on and created, FALSE otherwise
                        m_bRulersInitialized, // TRUE if the rulers have been dynamically allocated and intitialzed
                        m_bRulerTempOff,      // TRUE if ruler temporarily off for printing
                        m_bContinuePrinting;  // TRUE if more pages to print
    int                 m_iTimer;             // timer ID for modal background processing via CWaitDialog
    BOOL                m_bErrorState;        // set by CCriticalError::

    /*--- things for the "Find" dialog box  ---*/
    static CFindDlg     m_dlgFind;            // "find" dialog box (modeless) ... static so that there is only
                                              //        1 instance per application ... all views will share it
    CLPoint             m_ptlLastFind;        // row, col (char, not pixel) of last successful find
    BOOL                m_bShowLastFind;
    CFolio              m_folio;              // manages the header/footer and margins

// Operations
public:
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);

// Implementation
private:
    void SetScrollParameters();         // sets page, total sizes ... called from OnInitialUpdate and OnSize
    void ResizeRulers();                // causes ruler positioning to be redone
    void UpdateRulers();                // tells the rulers the current line and column position
    void UpdateStatusBar();             // causes the main frame to update the status bar current screen position
    BOOL OnScrollBy( CSize sizeScroll, BOOL bDoScroll); // csc 1/16/2005
//    void DoSearch()                   // performs the search, called via a timer set after the find dialog box

    void ChangeFontSize(bool increase); // 20110802


public:
    virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo = NULL);
    void UpdateVScrollPos (BOOL);
    void UpdateHScrollPos (BOOL);

    // Printing support
protected:
    virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo);

protected:
    // "find" dialog box
    LRESULT OnSearch(WPARAM wParam, LPARAM lParam);
    LRESULT OnSearchClose(WPARAM wParam, LPARAM lParam);

private:
    // "scanning file ... please wait" dialog box
    void ClearFindSelection();
    void SetLastFind (CLPoint ptlX) { m_ptlLastFind = ptlX;    }
    CLPoint GetLastFind()           { return m_ptlLastFind;    }
    BOOL ShowLastFind()             { return m_bShowLastFind;  }
    void SetShowLastFind (BOOL bX)  { m_bShowLastFind = bX;    }

// Overrides
    // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CTVView)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual void OnInitialUpdate();
    protected:
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //}}AFX_VIRTUAL

    void OnFilePrintPreview();

// Implementation
public:
        virtual ~CTVView();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    afx_msg void OnToggleRuler();

    //{{AFX_MSG(CTVView)
    afx_msg void OnEditCopy();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEditFind();
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnEditCopySS();
    afx_msg void OnUpdateViewRuler(CCmdUI* pCmdUI);
    afx_msg void OnViewGotoline();
    afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditCopySs(CCmdUI* pCmdUI);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnUpdateEditFind(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewGotoline(CCmdUI* pCmdUI);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnEditFindNext();
    afx_msg void OnUpdateEditFindNext(CCmdUI* pCmdUI);
    afx_msg void OnEditFindPrev();
    afx_msg void OnUpdateEditFindPrev(CCmdUI* pCmdUI);
    afx_msg void OnFileClose(); // 20110802 three new shortcuts
    afx_msg void OnFontBigger();
    afx_msg void OnFontSmaller();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in TVView.cpp
inline CTVDoc* CTVView::GetDocument()
   { return (CTVDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
