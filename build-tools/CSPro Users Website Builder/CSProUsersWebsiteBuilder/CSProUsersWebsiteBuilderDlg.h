#pragma once

#include <zUtilO/ResizableDlg.h>
#include <zUtilO/SettingsDb.h>


class CSProUsersWebsiteBuilderDlg : public ResizableDlg
{
public:
    CSProUsersWebsiteBuilderDlg(CWnd* pParent = nullptr);
    ~CSProUsersWebsiteBuilderDlg();

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnCancel() override;

    void OnUpdateGooglePlayPrivacyPolicy();

    LRESULT OnBuildTaskComplete(WPARAM wParam, LPARAM lParam);
    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    template<typename FP>
    void RunBuildTask(FP task_function);

private:
    SettingsDb m_settingsDb;
    std::string m_csproRootDirectory;
    std::string m_csproUsersInputDirectory;
    std::string m_csproUsersOutputDirectory;
    LoggingListBox m_loggingListBox;

    std::unique_ptr<std::thread> m_buildThread;
};
