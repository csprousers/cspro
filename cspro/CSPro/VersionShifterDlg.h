#pragma once


class VersionShifterDlg : public CDialog
{
public:
    enum { IDD = IDD_VERSION_SHIFTER };

    VersionShifterDlg(CWnd* pParent = nullptr);

    size_t GetNumVersions() const { return m_versionPaths.size(); }

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;

    void OnBnClickedOk();

private:
    std::vector<std::string> m_versionPaths;
    std::optional<size_t> m_currentIndex;
};
