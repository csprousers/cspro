#pragma once

//***************************************************************************
//  File name: GotoDlg.h
//
//  Description:
//       Interface for the CGotoDialog class
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1995        csc       created
//
//***************************************************************************

class CGotoDialog : public CDialog
{
// Construction
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    CGotoDialog(CWnd* pParent = NULL);  // standard constructor
    void SetView (CView* pV)  { m_pCurrView = pV;  }

// Dialog Data
    //{{AFX_DATA(CGotoDialog)
    enum { IDD = IDD_DLG_GOTO };
    long    m_LineNumber;
    //}}AFX_DATA

// Implementation
protected:
    CToolTipCtrl m_tooltip;
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CGotoDialog)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CView* m_pCurrView;
};
