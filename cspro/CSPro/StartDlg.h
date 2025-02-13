#pragma once

// CStartDlg dialog

class CStartDlg : public CDialog
{
    DECLARE_DYNAMIC(CStartDlg)

public:
    CStartDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CStartDlg();

// Dialog Data
    enum { IDD = IDD_START };
    HICON m_hIcon1;
    HICON m_hIcon2;
    CImageList m_cImageList;
    int m_iChoice;
    int m_iSelection;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnPaint();
    afx_msg void OnNMDblclkFileList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnSetfocusOpenExisting();
    afx_msg void OnBnSetfocusCreateNew();
    afx_msg void OnNMClickFileList(NMHDR *pNMHDR, LRESULT *pResult);
    virtual BOOL OnInitDialog();
protected:
    virtual void OnOK();
public:
    afx_msg void OnBnDoubleclickedCreateNew();
};
