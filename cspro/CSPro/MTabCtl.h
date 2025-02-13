#pragma once
// MTabCtl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMTabCtl window

#define DICT_TAB_LABEL      _T("Dicts")
#define FORM_TAB_LABEL      _T("Forms")
#define ORDER_TAB_LABEL     _T("Edits")
#define TABLE_TAB_LABEL     _T("Tables")
#define OBJECT_TAB_LABEL    _T("Files")

class CMTabCtl : public CTabCtrl
{
// Construction
public:
    CMTabCtl();

// Attributes
public:
    CImageList m_cImageList;
// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMTabCtl)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CMTabCtl();
    void  UpdateTabs();
    int   GetItemIndex(const CString& sString) const;
    void  InitImageList(void);


    // Generated message map functions
protected:
    //{{AFX_MSG(CMTabCtl)
    afx_msg void OnSelchange(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
