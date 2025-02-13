#pragma once

// ProgressDialog dialog

class ProgressDialog : public CDialog
{
    DECLARE_DYNAMIC(ProgressDialog)

public:
    ProgressDialog();   // standard constructor


    void ShowModeless(CString msgText, CWnd* pParent);
    virtual ~ProgressDialog();

    LRESULT Update(WPARAM wParam, LPARAM lParam);

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_PROGRESS_DIALOG };
#endif

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    void SetMessageText(CString msg);
    void SetUpFont();
    void LayoutControls();

    DECLARE_MESSAGE_MAP()
private:

    void runMessageLoop();
    CFont m_defaultFont;
    CFont* m_pFont;
    CString m_messageText;
    bool m_isCancelled;

public:
    afx_msg void OnBnClickedCancel();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};
