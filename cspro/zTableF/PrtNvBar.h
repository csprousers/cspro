#pragma once
//***************************************************************************
//  File name: PrtNvBar.h
//
//  Description:
//       Print view navigation dialog bar.
//
//***************************************************************************

#include <zTableF/zTableF.h>

/////////////////////////////////////////////////////////////////////////////
//
//                      CPrtViewNavigationBar dialog
//
/////////////////////////////////////////////////////////////////////////////
class CLASS_DECL_ZTABLEF CPrtViewNavigationBar : public CDialogBar
{
    DECLARE_DYNAMIC(CPrtViewNavigationBar)

// members
protected:
    CToolBar        m_wndToolBar;         // the navigation tool bar
    CStatic         m_staticPageNumber;   // page number static text box
    CFont           m_fontPageNumber;         // font for page number

// construction
public:
    CPrtViewNavigationBar();   // standard constructor
    virtual ~CPrtViewNavigationBar();

// access
public:
    CToolBar* GetToolBar(void) { return &m_wndToolBar; }
    CStatic* GetStaticText(void) { return &m_staticPageNumber; }


// implementation
public:
    void CenterToolBar(void);
    void SetPageInfo(int iCurrentPage, int iNumPages);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    afx_msg LONG OnInitDialog(UINT, LONG);
    afx_msg BOOL OnToolTipText(UINT nID, NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
};
