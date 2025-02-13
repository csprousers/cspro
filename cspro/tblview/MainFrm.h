#pragma once

#include <tblview/TVDlgBar.h>
#include <zUToolO/zUtoolO.h>
#include <zUtilO/BCMenu.h>

class ObjectTransporter;


class CMainFrame : public COXMDIFrameWndSizeDock
{
    DECLARE_DYNAMIC(CMainFrame)

public:
    CMainFrame();

    HMENU TblViewMenu();
    HMENU DefaultMenu();

// Attributes
public:
    BCMenu  m_DefaultMenu;
    BCMenu  m_TblViewMenu;
    LPCTSTR  m_pszClassName;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMainFrame();

    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);
    LRESULT ShowTabToolBar(WPARAM wParam,LPARAM lParam);
    LRESULT HideTabToolBar(WPARAM wParam,LPARAM lParam);
    LRESULT OnUpdateTree(WPARAM wParam,LPARAM lParam);
    LONG OnIMSAFileOpen(UINT, LPARAM);
    LONG OnIMSAFileClose(UINT, LPARAM);
    LONG OnIMSASetFocus(UINT, LPARAM);

    CToolBar*   m_pWndTabTBar;

protected:  // control bar embedded members
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;
    CReBar      m_wndReBar;
    CDialogBar      m_wndDlgBar;
    CComboBox   m_tabAreaComboBox;
    CComboBox   m_tabZoomComboBox;
    CButton     m_printViewCloseButton;

public:
    CTVDlgBar   m_SizeDlgBar;

    void OnViewNames();

// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()

    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    //}}AFX_MSG
    void OnUpdateAreaComboBox(CCmdUI* pCmdUI);
    void OnUpdateZoomComboBox(CCmdUI* pCmdUI);
    void OnUpdateViewNames(CCmdUI* pCmdUI);
    void OnUpdateQuickQuit(CCmdUI* pCmdUI);
    afx_msg void OnQuickQuit();
    afx_msg LRESULT OnDDEExecute(WPARAM wParam, LPARAM lParam);

private:
    std::unique_ptr<ObjectTransporter> m_objectTransporter;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
