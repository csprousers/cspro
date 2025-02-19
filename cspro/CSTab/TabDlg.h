#pragma once


class CSTabDlg : public CDialog
{
public:
    CSTabDlg(std::shared_ptr<CNPifFile> pff, std::string application_file_path, CWnd* pParent = nullptr);

    bool MakePifFile();

    bool CheckNCollectInputFiles();
    bool BuildPifInfo4Check();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    void OnSysCommand(UINT nID, LPARAM lParam);
    void OnPaint();
    HCURSOR OnQueryDragIcon();
    void OnLocate();

private:
    std::shared_ptr<CNPifFile> m_pff;
    CString m_sFileName;
    HICON m_hIcon;
};
