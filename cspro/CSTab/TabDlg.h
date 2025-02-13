#pragma once
// TabDlg.h : header file
//

// CSTabDlg dialog
class CSTabDlg : public CDialog
{
// Construction
public:
    CSTabDlg(CWnd* pParent = NULL); // standard constructor

public:
    CNPifFile*           m_pPIFFile;

public:
    bool    MakePifFile();
// Dialog Data
    enum { IDD = IDD_CSTAB_DIALOG };
    CString m_sFileName;

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    bool CheckNCollectInputFiles();
    bool BuildPifInfo4Check();



// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnLocate();
    virtual void OnOK();
    DECLARE_MESSAGE_MAP()
};
