#pragma once

// TDlgBar.h : header file
//
#include <zUToolO/zUtoolO.h>
#include <zTableF/TabTrCtl.h>

#define ID_FIXEDDLGBAR                  0xe881

/////////////////////////////////////////////////////////////////////////////
// CTVDlgBar dialog


class CTVDlgBar : public COXSizeDialogBar
{
// Construction
public:
    CTVDlgBar();

// Dialog Data
    //{{AFX_DATA(CTVDlgBar)
    enum { IDD = IDD_DIALOGBAR };
    CTabTreeCtrl    m_TblTree;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTVDlgBar)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CTVDlgBar)
    // NOTE: the ClassWizard will add member functions here
    virtual BOOL OnInitDialog();

    //}}AFX_MSG
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
