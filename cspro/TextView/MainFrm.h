#pragma once
//***************************************************************************
//  File name: MainFrm.h
//
//  Description:
//       Header for Main Frame of Text Viewer application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              18 Nov 99   bmd     Created for CSPro 2.0
//              28 Aug 00   gsf     OnIMSAFileClose: don't crash if file is not open
//              4  Jan 03   csc     Support for font size changes
//
//***************************************************************************

#include <zUtilO/BCMenu.h>
#include <TextView/Tvmisc.h>

// .ini file stuff
#define INI_SECTION_WINDOWSIZE      _T("Window size")
#define INI_KEY_RECT                  _T("Rect")
#define INI_KEY_ICON                  _T("Icon")
#define INI_KEY_MAX                   _T("Max")
#define INI_KEY_TOOL                  _T("Tool")
#define INI_KEY_STATUS                _T("Status")

class CMainFrame : public CMDIFrameWnd {

    DECLARE_DYNAMIC(CMainFrame)
public:
    CMainFrame();
    HMENU TextViewMenu();
    HMENU DefaultMenu();

// Members
public:
    BOOL    m_bKeepCommas;
    BOOL    m_bLineDraw;
    int     m_iPointSize;         // added csc 4 Jan 03
    CFont   m_Font;
    BOOL    m_bFirstTime;
    LPCTSTR  m_pszClassName;
    static const CRect NEAR rectDefault;

    BCMenu  m_TextViewMenu;
    BCMenu  m_DefaultMenu;

protected:
    // Control bar embedded members
    CStatusBar  m_wndStatusBar;
    CToolBar    m_wndToolBar;
    CReBar      m_wndReBar;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMainFrame();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
                CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

    void UpdateStatusBarScr (CLPoint);
    void UpdateStatusBarBlock (CLPoint, BOOL);
    void UpdateStatusBarSize (const TCHAR*);
    void UpdateStatusBarEncoding (const TCHAR*); // GHM 20111222

    int GetFontHeight(int iPointSize) const;     // added csc 4 Jan 03

    void OpenInDataViewer(const CString& filename);

// Generated message map functions
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnViewRuler();
    afx_msg void OnUpdateOpenInDataViewer(CCmdUI* pCmdUI);
    afx_msg void OnOpenInDataViewer();
    afx_msg void OnUpdateOptionsCommas(CCmdUI* pCmdUI);
    afx_msg void OnOptionsCommas();
    afx_msg void OnDestroy();
    afx_msg void OnQuickQuit();
    afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
    afx_msg void OnOptionsLinedraw();
    afx_msg void OnUpdateOptionsLinedraw(CCmdUI* pCmdUI);
    afx_msg void OnFormatFont();
    afx_msg void OnWindowNext();
    afx_msg void OnUpdateWindowNext(CCmdUI* pCmdUI);
    afx_msg void OnWindowPrev();
    afx_msg void OnUpdateWindowPrev(CCmdUI* pCmdUI);
    afx_msg void OnUpdateKeyOvr(CCmdUI* pCmdUI);
    afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    //}}AFX_MSG
    LONG OnIMSAFileOpen(UINT, LONG);
    LONG OnIMSAFileClose(UINT, LONG);
    LONG OnIMSASetFocus(UINT, LONG);
    afx_msg LRESULT OnDDEExecute(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
