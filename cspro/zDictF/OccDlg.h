#pragma once
// OccDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COccDlg dialog

#include <zDictF/OccGrid.h>

class CDDDoc;


class COccDlg : public CDialog
{
// Construction
public:
    COccDlg(CWnd* pParent = NULL);   // standard constructor
    COccGrid m_OccGrid;
    CRect m_rect;
    CDDDoc*     m_pDoc;
    CIMSAString m_sLabel;
// Dialog Data
    //{{AFX_DATA(COccDlg)
    enum { IDD = IDD_OCCURRENCE };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COccDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(COccDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
