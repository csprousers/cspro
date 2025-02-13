#pragma once
// MDlgBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMDlgBar dialog

#include <zUToolO/zUtoolO.h>
#include <zFormF/FrmTrCtl.H>
#include <zOrderF/OrdTrCtl.H>
#include <zTableF/TabTrCtl.h>
#include <CSPro/ObjTCtrl.h>
#include <CSPro/MTabCtl.h>


#define ID_FIXEDDLGBAR                  0xe881

class CMDlgBar :  public COXSizeDialogBar
{
// Construction
public:
    DECLARE_DYNAMIC(CMDlgBar)

    CMDlgBar(CWnd* pParent = NULL);   // standard constructor
    void UpdateTabs();
    void SelectTab(CString sTabName, int iTabMode);      //Selects the tab from the tab control
    void SelectTab(CWnd* pWnd);      //Selects the tab from the tab control by ptr to window for the tab contents
    void UpdateTrees();
    void GetTabNames(CStringArray& aNames) const;
    void GetTabWindows(CArray<CWnd*>& aWnds);
    CWnd* NameToWindow(LPCTSTR lpszName);
    CIMSAString WindowToName(CWnd* pWnd) const;


// Dialog Data
    //{{AFX_DATA(CMDlgBar)
    enum { IDD = IDD_DIALOGBAR };
    CFormTreeCtrl   m_FormTree;
    COrderTreeCtrl  m_OrderTree;
    CTabTreeCtrl    m_TableTree;
    CDDTreeCtrl     m_DictTree;
    CMTabCtl        m_tabCtl;
    CObjTreeCtrl    m_ObjTree;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMDlgBar)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CMDlgBar)
        virtual BOOL OnInitDialog();
    afx_msg void OnWindowPosChanging (LPWINDOWPOS lpWndPos);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
