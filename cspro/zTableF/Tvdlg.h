#pragma once

//***************************************************************************
//  File name: TVDlg.h
//
//  Description:
//       Interface for the TableView dialog classes
//
//  History:    Date       Author     Comment
//              -----------------------------
//              1996         csc      created
//              10 dec 96    csc      save/print help context in CSelectionDialog
//              26 sep 97    gsf      change checklist box to list box
//
//***************************************************************************

class CTabulateDoc;

/////////////////////////////////////////////////////////////////////////////
// CPickTablesDlg dialog
class CPickTablesDlg : public CDialog {
public:
    CTabulateDoc*       m_pDoc;

private:
    int                 m_iIDTitle;       // dlg title ("save","print") is from a resource at init time
//    CListBox            m_checklistbox;    // keep the old name; it no longer uses checks; gsf 09/26/97
    CStringArray        m_acsList;
    CArray<BOOL, BOOL>  m_aiCheck;

// Construction
public:
    CPickTablesDlg(int iIDTitle, CWnd* pParent = NULL);   // standard constructor
    void AddString(const CIMSAString& cs)  { m_acsList.Add(cs);  }
    void SetCheck(int iIndex, int iCheck);
    int GetCheck(int iIndex);

// Dialog Data
    //{{AFX_DATA(CPickTablesDlg)
    enum { IDD = IDD_PICK_TABLES };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPickTablesDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    BOOL OnInitDialog();

    // Generated message map functions
    //{{AFX_MSG(CPickTablesDlg)
    virtual void OnOK();
    afx_msg void OnCheckall();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

