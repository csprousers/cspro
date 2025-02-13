#pragma once
//***************************************************************************
//  File name: CSSort.h
//
//  Description:
//       Header for CSSort application
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

/////////////////////////////////////////////////////////////////////////////
// CSortApp:
// See CSSort.cpp for the implementation of this class
//

class CSortApp : public CWinApp
{
public:
    int                 m_iReturnCode;

private:
    CString             m_csModuleName;
    HICON               m_hIcon;

public:
    CSortApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSortApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation
    //{{AFX_MSG(CSortApp)
    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
