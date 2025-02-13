#pragma once

// GHM 20120817

// CKeyboardInputDlg dialog

class CKeyboardInputDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CKeyboardInputDlg)

    CListCtrl * m_pList;
    UINT m_KLID;
    UINT m_SelectedKLID;

public:
    CKeyboardInputDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CKeyboardInputDlg();

    void SetKLID(UINT klid) { m_KLID = klid; }
    UINT GetKLID() { return m_SelectedKLID; }

    static CString GetDisplayName(UINT klid);
    static CString GetDisplayNameHKL(HKL hKL);
    static UINT HKL2KLID(HKL hKL);
    static UINT LayoutName2KLID(TCHAR * pLayoutName);
    static void KLID2LayoutName(UINT klid,TCHAR * pLayoutName);

// Dialog Data
    enum { IDD = IDD_KEYBOARD_LAYOUTS };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnLvnItemchangedKeyboardList(NMHDR *pNMHDR, LRESULT *pResult);
};
