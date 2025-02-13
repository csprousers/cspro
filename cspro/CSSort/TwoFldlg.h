#pragma once

//***************************************************************************
//  File name: Twofldlg.h
//
//  Description:
//       Header for CSSort program two file dialog
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

class CTwoFileDialog : public CDialog
{
// Construction
public:
    CTwoFileDialog(PFF& pff, CString dictionary_filename, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CTwoFileDialog)
    enum { IDD = IDD_TWOFILE };
    //}}AFX_DATA

private:
    PFF& m_pff;
    CString m_dictionaryFilename;
    CIMSAString m_csInFileName;
    CIMSAString m_csOutFileName;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CTwoFileDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CTwoFileDialog)
    afx_msg void OnChangeFileName();
    afx_msg void OnKillfocusInFileName();
    afx_msg void OnInFileBrowse();
    afx_msg void OnOutFileBrowse();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
