#pragma once

class CPromptFunctionDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CPromptFunctionDlg)

private:
    CFont m_Font;
    CEdit* m_pResponseEdit;

    CString m_csTitle;
    CString m_csInitialResponse;
    CString m_csResponse;

    bool m_bMultiline;
    bool m_bNumeric;
    bool m_bPassword;
    bool m_bUpperCase;

public:
    CPromptFunctionDlg(const CString& csTitle,const CString& csInitialResponse,bool bMultiline,bool bNumeric,bool bPassword,bool bUpperCase);
    virtual ~CPromptFunctionDlg();

    CString GetResponse() const { return m_csResponse; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    DECLARE_MESSAGE_MAP()
};
