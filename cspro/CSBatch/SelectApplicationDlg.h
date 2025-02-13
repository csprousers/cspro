#pragma once

class SelectApplicationDlg : public CDialog
{
public:
    SelectApplicationDlg(CWnd* pParent = nullptr);

    const CString& GetApplicationFilename() const { return m_applicationFilename; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnSysCommand(UINT nID, LPARAM lParam);

    void OnSelectApplicationFilename();    

private:
    HICON m_hIcon;
    CString m_applicationFilename;
};
