#pragma once

//***************************************************************************
//  File name: SelDlg.h
//
//  Description:
//       Interface for the CSelectionDialog class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//
//***************************************************************************

class CSelectionDialog : public CDialog
{
// Construction
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    CSelectionDialog(CWnd* pParent = NULL); // standard constructor
    void SetView (CView* pV)  { m_pView = pV;  }

// Dialog Data
    //{{AFX_DATA(CSelectionDialog)
    enum { IDD = IDD_SAVEAS_SEL };
    int     m_nAll;
    //}}AFX_DATA

// Implementation
protected:
    CToolTipCtrl m_tooltip;
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CSelectionDialog)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CView* m_pView;
};
