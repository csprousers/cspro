#pragma once

#include <zUtilO/ResizableDlg.h>


class SettingsDlg : public ResizableDlg
{
public:
    SettingsDlg(CWnd* pParent = nullptr);

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

private:
    Controller& m_controller;

    std::string m_openSourceCodeDirectory;
    std::string m_openSourceLibrariesDirectory;
    std::string m_githubPAT;
};
