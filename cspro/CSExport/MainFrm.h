#pragma once

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include <zUtilO/BCMenu.h>
#include <zInterfaceF/LangDlgBar.h>


class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
    CMainFrame();
    DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
    BOOL    m_bFixPaint;
    CRect   m_rcMinSize;

// Operations
public:
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    //}}AFX_VIRTUAL

// Implementation
public:
    BCMenu m_menu;
    virtual ~CMainFrame();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

private:
    bool PostRunFileCheck(CString csFilename);

protected:  // control bar embedded members
    CReBar      m_wndReBar;
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;
    LangDlgBar  m_wndLangDlgBar;

public:
    const CReBar& GetReBar()    { return m_wndReBar; }
    LangDlgBar& GetLangDlgBar() { return m_wndLangDlgBar; }

// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()

    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    afx_msg void OnUpdateFileRun(CCmdUI* pCmdUI);
    afx_msg void OnFileRun();
    //}}AFX_MSG

    void OnUpdateFrameTitle(BOOL bAddToTitle) override;

    LRESULT OnSelectLanguage(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetLexerLanguage(WPARAM wParam, LPARAM lParam);

    LONG OnIMSAExportDone(UINT, LPARAM);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
