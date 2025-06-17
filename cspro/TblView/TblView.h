#pragma once
// TblView.h : main header file for the TBLVIEW application
//

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif


/////////////////////////////////////////////////////////////////////////////
// CTblViewApp:
// See TblView.cpp for the implementation of this class
//
class CTblViewDoc;
class CTblViewApp : public CWinApp
{
public:
    CTblViewApp();
    CString             m_csModuleName;
    HICON               m_hIcon;
    CString             m_csWndClassName;
    BOOL                m_bCalledAsChild;  // TRUE when we are invoked with the /c command line switch


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTblViewApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CTblViewApp)
    afx_msg void OnAppAbout();
    afx_msg void OnFileNew();
    afx_msg void OnFullscreen();
    afx_msg void OnUpdateFullscreen(CCmdUI* pCmdUI);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    afx_msg void OnFileOpen();
    afx_msg void OnFileSaveAs();
    virtual int ExitInstance();
public:
    CTblViewDoc* FindFile(const CString& csFileName) const;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
