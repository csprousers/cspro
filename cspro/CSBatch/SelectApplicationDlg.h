#pragma once


class SelectApplicationDlg : public CDialog
{
public:
    SelectApplicationDlg(CWnd* pParent = nullptr);

    const std::string& GetApplicationFilePath() const { return m_applicationFilePath; }

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnSysCommand(UINT nID, LPARAM lParam);

    void OnSelectApplicationFilePath();

private:
    HICON m_hIcon;
    std::string m_applicationFilePath;
};
